#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <chrono>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>

//proj
#include "hashtable.h"
#include "list.h"
#include "heap.h"
#include "thread_pool.h"
#include "common.h"

using namespace std::chrono;
using std::cout;
using std::cerr;
using std::endl;

static void msg(const char *msg) {
    cerr << msg << endl;
}

static void die(const char *msg) {
    int err = errno;
    cerr << "[" << err << "] " << msg << endl;
    abort();
}

static uint64_t get_monotonic_usec() {
    timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000000 + tv.tv_nsec / 1000;
}

static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

struct Conn;

// global variables
static struct GlobalData {
    HMap db;
    // a map of all client connections, keyed by fd
    std::vector<std::unique_ptr<Conn>> fd2conn;
    // timers for idle connections
    DList idle_list;
    // timers for TTLs
    std::vector<HeapItem> heap;
    // the thread pool
    ThreadPool tp;
} g_data;

const size_t k_max_msg = 4096;

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,  // mark the connection for deletion
};

struct Conn {
    int fd = -1;
    uint32_t state = 0;     // either STATE_REQ or STATE_RES
    // buffer for reading
    size_t rbuf_size = 0;
    std::vector<uint8_t> rbuf;
    // buffer for writing
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    std::vector<uint8_t> wbuf;
    uint64_t idle_start = 0;
    // timer
    DList idle_list;
};

static void conn_put(std::vector<std::unique_ptr<Conn>> &fd2conn, std::unique_ptr<Conn> conn) {
    if (fd2conn.size() <= static_cast<size_t>(conn->fd)) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = std::move(conn);
}

static int32_t accept_new_conn(int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    auto conn = std::make_unique<Conn>();
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn->idle_start = get_monotonic_usec();
    dlist_insert_before(&g_data.idle_list, &conn->idle_list);
    conn_put(g_data.fd2conn, std::move(conn));
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        die("setsockopt()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);

    if (bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
        die("bind()");
    }

    if (listen(fd, SOMAXCONN) < 0) {
        die("listen()");
    }

    fd_set_nb(fd);

    dlist_init(&g_data.idle_list);
    thread_pool_init(&g_data.tp, 4);

    std::vector<struct pollfd> poll_args;
    while (true) {
        poll_args.clear();
        struct pollfd pfd = { fd, POLLIN, 0 };
        poll_args.push_back(pfd);
        for (const auto& conn : g_data.fd2conn) {
            if (conn) {
                struct pollfd pfd = {};
                pfd.fd = conn->fd;
                pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
                pfd.events = pfd.events | POLLERR;
                poll_args.push_back(pfd);
            }
        }

        int rv = poll(poll_args.data(), static_cast<nfds_t>(poll_args.size()), -1);
        if (rv < 0) {
            die("poll");
        }

        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) {
                Conn* conn = g_data.fd2conn[i - 1].get();
                connection_io(conn);
                if (conn->state == STATE_END) {
                    conn_done(conn);
                }
            }
        }

        process_timers();

        if (poll_args[0].revents) {
            (void)accept_new_conn(fd);
        }
    }

    return 0;
}


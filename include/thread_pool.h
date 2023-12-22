#pragma once

#include <cstddef>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>

struct Work {
    std::function<void()> f=nullptr;
    void* arg = nullptr;
};

struct ThreadPool {
    std::vector<std::thread> threads;
    std::deque<Work> queue;
    std::mutex mu;
    std::condition_variable not_empty;
    bool stop = false;  // Flag to signal threads to stop

    // Destructor to join threads when the ThreadPool object is destroyed
    ~ThreadPool() {
        stop = true;
        not_empty.notify_all();
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
};

void thread_pool_init(ThreadPool& tp, size_t num_threads);
void thread_pool_queue(ThreadPool& tp, std::function<void()> f, void* arg);

#include <cassert>
#include <functional>
#include "thread_pool.h"


static void worker(ThreadPool* tp) {
    while (true) {
        std::unique_lock<std::mutex> lock(tp->mu);
        tp->not_empty.wait(lock, [tp] { return !tp->queue.empty() || tp->stop; });

        if (tp->stop && tp->queue.empty()) {
            return;
        }

        Work w = tp->queue.front();
        tp->queue.pop_front();
        lock.unlock();

        w.f(); // Call the function without an argument since it's captured in the lambda.
    }
}

void thread_pool_init(ThreadPool& tp, size_t num_threads) {
    assert(num_threads > 0);

    tp.threads.resize(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        tp.threads[i] = std::thread(worker, &tp);
    }
}

void thread_pool_queue(ThreadPool& tp, std::function<void()> f, void* arg) {
    {
        std::lock_guard<std::mutex> lock(tp.mu);
        tp.queue.emplace_back(Work{std::move(f), arg});
    }
    tp.not_empty.notify_one();
}

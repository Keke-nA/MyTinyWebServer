#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

class ThreadPool {
  public:
    ThreadPool(const size_t threadpoolsize = 8)
        : thread_pool(std::make_shared<Pool>()) {
        for (size_t i = 0; i < threadpoolsize; i++)
            std::thread([temp_pool = thread_pool] {
                std::unique_lock<std::mutex> lock(temp_pool->pool_mtx);
                while (true) {
                    // 改进：防止虚假唤醒
                    while (!temp_pool->pool_is_closed
                           && temp_pool->pool_tasks.empty()) {
                        temp_pool->pool_cv.wait(lock);
                    }

                    if (temp_pool->pool_is_closed
                        && temp_pool->pool_tasks.empty()) {
                        break;
                    }

                    if (!temp_pool->pool_tasks.empty()) {
                        auto task = std::move(temp_pool->pool_tasks.front());
                        temp_pool->pool_tasks.pop();
                        lock.unlock();
                        task();
                        lock.lock();
                    }
                }
            }).detach();
    }
    ThreadPool(ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) noexcept = delete;
    ThreadPool& operator=(ThreadPool&&) noexcept = delete;
    ~ThreadPool() {
        thread_pool->pool_is_closed = true;
        thread_pool->pool_cv.notify_all();
    }

    template <typename T> void addTask(T&& task) {
        {
            std::lock_guard<std::mutex> lock(thread_pool->pool_mtx);
            if (thread_pool->pool_is_closed) {
                throw std::runtime_error("ThreadPool is closed");
            }
            thread_pool->pool_tasks.emplace(std::forward<T>(task));
        }
        thread_pool->pool_cv.notify_all();
    }

  private:
    class Pool {
      public:
        Pool() : pool_is_closed(false) {
        }
        std::mutex pool_mtx;
        std::condition_variable pool_cv;
        std::atomic_bool pool_is_closed;
        std::queue<std::function<void()>> pool_tasks;
    };
    std::shared_ptr<Pool> thread_pool;
};
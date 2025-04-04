#pragma once
#include <assert.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>

// 模板类 BlockDeque，用于实现一个线程安全的阻塞双端队列
template <typename T>
class BlockDeque {
   public:
    // 构造函数，初始化阻塞双端队列，可指定最大容量，默认 1000
    BlockDeque(size_t maxcapacity = 1000) : bd_capacity(maxcapacity), bd_isclose(false) { assert(maxcapacity > 0); }
    // 析构函数，清理资源
    ~BlockDeque() { close(); }
    // 清空队列中的所有元素
    void clear() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        block_dq.clear();
    }
    // 判断队列是否为空
    bool empty() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        return block_dq.empty();
    }
    // 判断队列是否已满
    bool full() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        return block_dq.size() == bd_capacity;
    }
    // 关闭队列，唤醒所有等待的线程
    void close() {
        {
            std::lock_guard<std::mutex> lock(bd_mtx);
            block_dq.clear();
            bd_isclose = true;
        }
        bd_consumer_cv.notify_all();
        bd_produser_cv.notify_all();
    }
    // 获取队列当前的元素数量
    size_t size() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        return block_dq.size();
    }
    // 获取队列的最大容量
    size_t capacity() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        return bd_capacity;
    }
    // 获取队列的队首元素
    T front() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        assert(block_dq.size() > 0);
        return block_dq.front();
    }
    // 获取队列的队尾元素
    T back() {
        std::lock_guard<std::mutex> lock(bd_mtx);
        assert(block_dq.size() > 0);
        return block_dq.back();
    }
    // 在队列的队尾插入一个元素
    void push_back(const T& item) {
        std::unique_lock<std::mutex> lock(bd_mtx);
        bd_produser_cv.wait(lock, [this]() { return block_dq.size() < bd_capacity || bd_isclose; });
        if (bd_isclose)
            return;  // 若关闭则放弃插入
        block_dq.push_back(item);
        bd_consumer_cv.notify_one();
    }
    // 在队列的队首插入一个元素
    void push_front(const T& item) {
        std::unique_lock<std::mutex> lock(bd_mtx);
        bd_produser_cv.wait(lock, [this]() { return block_dq.size() < bd_capacity || bd_isclose; });
        if (bd_isclose)
            return;
        block_dq.push_front(item);
        bd_consumer_cv.notify_one();  // 通知消费者
    }
    // 从队列的队首取出一个元素
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(bd_mtx);
        bd_consumer_cv.wait(lock, [this]() { return !block_dq.empty() || bd_isclose; });
        if (bd_isclose && block_dq.empty())
            return false;
        item = std::move(block_dq.front());  // 直接访问元素，避免调用 front()
        block_dq.pop_front();
        bd_produser_cv.notify_one();
        return true;
    }
    // 从队列的队首取出一个元素，带有超时机制
    bool pop(T& item, size_t timeout_ms) {
        std::unique_lock<std::mutex> lock(bd_mtx);
        auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        // 使用条件变量的 wait_until + 谓词
        if (!bd_consumer_cv.wait_until(lock, timeout, [this]() { return !block_dq.empty() || bd_isclose; })) {
            return false;  // 超时
        }
        if (bd_isclose && block_dq.empty())
            return false;
        item = std::move(block_dq.front());
        block_dq.pop_front();
        bd_produser_cv.notify_one();
        return true;
    }
    // 唤醒一个等待在队列上的消费者线程
    void flush() { bd_consumer_cv.notify_one(); }

   private:
    std::deque<T> block_dq;                  // 存储元素的双端队列
    size_t bd_capacity;                      // 队列的最大容量
    std::mutex bd_mtx;                       // 互斥锁，用于保证线程安全
    std::atomic_bool bd_isclose;             // 标志位，用于表示队列是否关闭
    std::condition_variable bd_consumer_cv;  // 消费者条件变量，用于线程等待
    std::condition_variable bd_produser_cv;  // 生产者条件变量，用于线程等待
};
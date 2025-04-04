#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include "BlockDeque.hpp"

using namespace std;

void TestBasic() {
    cout << "Running Basic Test..." << endl;
    BlockDeque<int> deque(3);

    deque.push_back(1);
    deque.push_front(2);
    assert(deque.size() == 2);
    assert(deque.front() == 2);
    assert(deque.back() == 1);

    int item;
    assert(deque.pop(item));  // 应该得到2
    assert(item == 2);
    assert(deque.pop(item));  // 得到1
    assert(deque.empty());

    cout << "Basic Test Passed!" << endl;
}

void TestFullQueue() {
    cout << "Running FullQueue Test..." << endl;
    BlockDeque<int> deque(2);

    deque.push_back(1);
    deque.push_back(2);  // 现在已满

    thread producer([&deque] {
        deque.push_back(3);  // 应被阻塞
    });

    // 等待一段时间确保生产者进入等待状态
    this_thread::sleep_for(chrono::milliseconds(100));
    assert(deque.size() == 2);  // 生产者未成功插入

    deque.close();
    producer.join();
    assert(deque.empty());  // 关闭后清空

    cout << "FullQueue Test Passed!" << endl;
}

void TestTimeout() {
    cout << "Running Timeout Test..." << endl;
    BlockDeque<int> deque(1);
    deque.close();

    int item;
    assert(!deque.pop(item, 100));  // 立即返回false
    deque.clear();
    deque.close();

    cout << "Timeout Test Passed!" << endl;
}

void TestMultiThread() {
    cout << "Running MultiThread Test..." << endl;
    const size_t capacity = 100;
    const size_t num_producers = 5;
    const size_t num_consumers = 3;
    const size_t total_items = 10000;

    BlockDeque<int> deque(capacity);
    atomic<size_t> produced(0);
    atomic<size_t> consumed(0);
    atomic<bool> start(false);

    auto producer = [&](size_t id) {
        while (!start)
            this_thread::yield();
        while (produced < total_items) {
            int item = produced.fetch_add(1);
            deque.push_back(item);
        }
    };

    auto consumer = [&](size_t id) {
        while (!start)
            this_thread::yield();
        while (true) {
            int item;
            if (!deque.pop(item))
                break;  // 队列关闭退出
            consumed++;
        }
    };

    vector<thread> producers;
    vector<thread> consumers;

    for (size_t i = 0; i < num_producers; i++) {
        producers.emplace_back(producer, i);
    }
    for (size_t i = 0; i < num_consumers; i++) {
        consumers.emplace_back(consumer, i);
    }

    // 启动测试
    start = true;

    // 等待生产者完成
    for (auto& t : producers)
        t.join();

    // 关闭队列让消费者退出
    deque.close();

    // 等待消费者完成
    for (auto& t : consumers)
        t.join();

    assert(consumed == total_items);
    assert(deque.empty());

    cout << "MultiThread Test Passed!" << endl;
}

int main() {
    try {
        TestBasic();
        TestFullQueue();
        TestTimeout();
        TestMultiThread();
    } catch (const exception& e) {
        cerr << "Test Failed: " << e.what() << endl;
        return 1;
    }

    cout << "All Tests Passed!" << endl;
    return 0;
}
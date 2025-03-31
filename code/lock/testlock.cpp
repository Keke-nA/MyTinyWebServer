#include <chrono>
#include <iostream>
#include <queue>
#include <thread>
#include "mylocker.h"

bool producers_done = false;
MyLocker lock;
MyCond cond;
std::queue<int> q;
const int QMAX = 10;

void producer(int x) {
    lock.myLock();
    while (q.size() >= QMAX) {
        cond.myWait(lock.myMutexGet());
    }
    q.push(x);
    std::cout << "producer:" << x << std::endl;
    cond.mySignal();
    lock.myUnlock();
}

void consumer() {
    while (true) {
        lock.myLock();
        while (q.empty() && !producers_done) {
            cond.myWait(lock.myMutexGet());
        }
        if (q.empty() && producers_done) {
            lock.myUnlock();
            break;
        }
        int top = q.front();
        q.pop();
        std::cout << "consumer:" << top << std::endl;
        cond.myBroadcast();
        lock.myUnlock();
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int main() {
    constexpr int num_producers = 5;
    constexpr int num_consumers = 2;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < num_producers; i++) {
        producers.emplace_back([=](int id) {
            for (int x = id; x < 30; x += num_producers) {
                producer(x);
            }
        }, i);
    }
    for (int i = 0; i < num_consumers; i++) {
        consumers.emplace_back(consumer);
    }

    for (auto& t : producers) {
        t.join();
    }

    lock.myLock();
    producers_done = true;
    cond.myBroadcast();
    lock.myUnlock();

    for (auto& t : consumers) {
        t.join();
    }
    return 0;
}
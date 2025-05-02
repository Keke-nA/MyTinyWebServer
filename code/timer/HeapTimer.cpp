#include "HeapTimer.hpp"
#include <cassert>
#include <cstddef>

// 构造函数
HeapTimer::HeapTimer()
    : heap_timer(std::vector<TimerNode>()),
      heap_ref(std::unordered_map<size_t, size_t>()) {
    // 初始化定时器堆和索引映射
}

// 析构函数
HeapTimer::~HeapTimer() {
    // 清理资源
}

// 调整定时器过期时间
void HeapTimer::adjust(size_t id, size_t newexpires) {
    assert(!heap_timer.empty() && heap_ref.count(id) > 0);
    // 更新过期时间
    heap_timer[heap_ref[id]].expires =
        std::chrono::high_resolution_clock::now() + static_cast<ms>(newexpires);
    // 调整堆结构，保持最小堆性质
    if (!siftDown(heap_ref[id])) {
        siftUp(heap_ref[id]);
    }
}

// 添加定时器节点
void HeapTimer::addTimeNode(size_t id, size_t timeout, timeOutCallBack cb) {
    assert(id > 0);
    if (heap_ref.count(id) == 0) {
        // 新增定时器节点
        heap_timer.emplace_back(
            id,
            std::chrono::high_resolution_clock::now()
                + static_cast<ms>(timeout),
            cb);
        heap_ref[id] = heap_timer.size() - 1;
        // 向上调整堆
        siftUp(heap_timer.size() - 1);
    } else {
        // 更新已有定时器节点
        int i = heap_ref[id];
        heap_timer[i].expires = std::chrono::high_resolution_clock::now()
                                + static_cast<ms>(timeout);
        heap_timer[i].cb = cb;
        // 调整堆结构
        if (!siftDown(i)) {
            siftUp(i);
        }
    }
}

// 执行指定定时器任务
void HeapTimer::doWork(size_t id) {
    if (heap_timer.empty() || heap_ref.count(id) == 0) {
        return;
    }
    // 执行回调函数
    heap_timer[heap_ref[id]].cb();
    // 删除已执行的定时器
    del(heap_ref[id]);
}

// 清空定时器
void HeapTimer::clear() {
    heap_timer.clear();
    heap_ref.clear();
}

// 处理到期的定时器
void HeapTimer::tick() {
    // 循环处理所有到期的定时器
    while (!heap_timer.empty()) {
        TimerNode node = heap_timer.front();
        // 检查是否到期
        if (std::chrono::duration_cast<ms>(
                node.expires - std::chrono::high_resolution_clock::now())
                .count()
            > 0) {
            break;
        }
        // 执行回调函数
        node.cb();
        // 移除已处理的定时器
        pop();
    }
}

// 移除堆顶定时器
void HeapTimer::pop() {
    if (!heap_timer.empty()) {
        del(0);
    }
}

// 获取下一个定时器的剩余时间
int HeapTimer::getNextTick() {
    tick();
    int next_time = -1;
    if (!heap_timer.empty()) {
        // 计算堆顶定时器的剩余时间
        next_time = std::chrono::duration_cast<ms>(
                        (heap_timer.front().expires
                         - std::chrono::high_resolution_clock::now()))
                        .count();
        if (next_time < 0) {
            next_time = 0;
        }
    }
    return next_time;
}

// 删除指定位置的定时器
void HeapTimer::del(size_t i) {
    assert(i < heap_timer.size());
    // 将要删除的节点与最后一个节点交换
    swapNode(i, heap_timer.size() - 1);
    // 移除索引映射
    heap_ref.erase(heap_timer[heap_timer.size() - 1].id);
    // 删除最后一个节点
    heap_timer.pop_back();
    // 调整堆结构
    if (!heap_timer.empty() && !siftDown(i)) {
        siftUp(i);
    }
}

// 向上调整堆
void HeapTimer::siftUp(size_t i) {
    assert(i < heap_timer.size());
    int j = (i - 1) / 2; // 父节点索引
    // 如果当前节点的过期时间小于父节点，则交换
    while (j >= 0 && heap_timer[i].expires < heap_timer[j].expires) {
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

// 向下调整堆
/*bool HeapTimer::siftDown(size_t i) {
    assert(i < heap_timer.size());
    size_t raw_i = i;
    int j = i * 2 + 1;
    while (j < heap_timer.size()) {
        if (j + 1 < heap_timer.size() && heap_timer[j].expires > heap_timer[j +
1].expires) { j++;
        }
        if (heap_timer[i].expires > heap_timer[j].expires) {
            swapNode(i, j);
            i = j;
            j = i * 2 + 1;
        } else {
            break;  // 如果不需要交换，跳出循环
        }
    }
    return raw_i > i;
}*/
bool HeapTimer::siftDown(size_t i) {
    assert(i < heap_timer.size());
    size_t left = i * 2 + 1;  // 左子节点
    size_t right = i * 2 + 2; // 右子节点
    size_t extremnum = i;     // 最小值节点索引
    // 找出当前节点、左子节点和右子节点中的最小值
    if (left < heap_timer.size()
        && heap_timer[left].expires < heap_timer[extremnum].expires) {
        extremnum = left;
    }
    if (right < heap_timer.size()
        && heap_timer[right].expires < heap_timer[extremnum].expires) {
        extremnum = right;
    }
    // 如果当前节点不是最小值，则交换并继续向下调整
    if (extremnum != i) {
        swapNode(i, extremnum);
        siftDown(extremnum);
        return true;
    }
    return false;
}

// 交换两个节点
void HeapTimer::swapNode(size_t i, size_t j) {
    assert(i < heap_timer.size() && j < heap_timer.size());
    if (i != j) {
        // 交换节点位置
        std::swap(heap_timer[i], heap_timer[j]);
        // 更新索引映射
        heap_ref[heap_timer[i].id] = i;
        heap_ref[heap_timer[j].id] = j;
    }
}

// 定时器节点比较运算符
bool TimerNode::operator<(TimerNode& t) {
    return expires < t.expires;
}
#include "HeapTimer.hpp"  // 请替换为你的头文件名称

// 构造函数，初始化堆定时器
HeapTimer::HeapTimer() : heap_timer(std::vector<TimerNode>()), heap_ref(std::unordered_map<size_t, size_t>()) {
    // 初始化代码
}

// 析构函数，清理堆定时器资源
HeapTimer::~HeapTimer() {
    // 清理代码
}

// 调整指定ID的定时器的过期时间
void HeapTimer::adjust(size_t id, size_t newexpires) {
    assert(!heap_timer.empty() && heap_ref.count(id) > 0);
    heap_timer[heap_ref[id]].expires = std::chrono::high_resolution_clock::now() + static_cast<ms>(newexpires);
    if (!siftDown(heap_ref[id])) {
        siftUp(heap_ref[id]);
    }
}

// 添加一个新的定时器节点
void HeapTimer::addTimeNode(size_t id, size_t timeout, timeOutCallBack cb) {
    assert(id > 0);
    if (heap_ref.count(id) == 0) {
        heap_timer.emplace_back(id, std::chrono::high_resolution_clock::now() + static_cast<ms>(timeout), cb);
        heap_ref[id] = heap_timer.size() - 1;
        siftUp(heap_timer.size() - 1);
    } else {
        int i = heap_ref[id];
        heap_timer[i].expires = std::chrono::high_resolution_clock::now() + static_cast<ms>(timeout);
        heap_timer[i].cb = cb;
        if (!siftDown(i)) {
            siftUp(i);
        }
    }
}

// 执行指定ID的定时器任务
void HeapTimer::doWork(size_t id) {
    if (heap_timer.empty() || heap_ref.count(id) == 0) {
        return;
    }
    heap_timer[heap_ref[id]].cb();
    del(heap_ref[id]);
}

// 清除所有定时器节点
void HeapTimer::clear() {
    heap_timer.clear();
    heap_ref.clear();
}

// 检查并执行过期的定时器任务
void HeapTimer::tick() {
    while (!heap_timer.empty()) {
        TimerNode node = heap_timer.front();
        if (std::chrono::duration_cast<ms>(node.expires - std::chrono::high_resolution_clock::now()).count() > 0) {
            break;
        }
        node.cb();
        pop();
    }
}

// 移除堆顶的定时器节点
void HeapTimer::pop() {
    if (!heap_timer.empty()) {
        del(0);
    }
}

// 获取下一个定时器任务的剩余时间
int HeapTimer::getNextTick() {
    tick();
    int next_time = -1;
    if (!heap_timer.empty()) {
        next_time =
            std::chrono::duration_cast<ms>((heap_timer.front().expires - std::chrono::high_resolution_clock::now()))
                .count();
        if (next_time < 0) {
            next_time = 0;
        }
    }
    return next_time;
}

// 删除指定索引的定时器节点
void HeapTimer::del(size_t i) {
    assert(i < heap_timer.size());
    swapNode(i, heap_timer.size() - 1);
    heap_ref.erase(heap_timer[heap_timer.size() - 1].id);
    heap_timer.pop_back();
    if (!heap_timer.empty() && !siftDown(i)) {
        siftUp(i);
    }
}

// 向上调整堆，确保堆的性质
void HeapTimer::siftUp(size_t i) {
    assert(i < heap_timer.size());
    int j = (i - 1) / 2;
    while (j >= 0 && heap_timer[i].expires < heap_timer[j].expires) {
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

// 向下调整堆，确保堆的性质
bool HeapTimer::siftDown(size_t i) {
    assert(i < heap_timer.size());
    size_t raw_i = i;
    int j = i * 2 + 1;
    while (j < heap_timer.size()) {
        if (j + 1 < heap_timer.size() && heap_timer[j].expires > heap_timer[j + 1].expires) {
            j++;
        }
        if (heap_timer[i].expires > heap_timer[j].expires) {
            swapNode(i, j);
            i = j;
        }
        j = i * 2 + 1;
    }
    return raw_i > i;
}

// 交换两个索引对应的定时器节点
void HeapTimer::swapNode(size_t i, size_t j) {
    assert(i < heap_timer.size() && j < heap_timer.size());
    if (i != j) {
        std::swap(heap_timer[i], heap_timer[j]);
        heap_ref[heap_timer[i].id] = i;
        heap_ref[heap_timer[j].id] = j;
    }
}

// 重载小于运算符，用于比较两个定时器节点的过期时间
bool TimerNode::operator<(TimerNode& t) {
    return expires < t.expires;
}
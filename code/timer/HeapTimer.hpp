#include <assert.h>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

// 定义超时回调函数类型，该函数无参数且无返回值
using timeOutCallBack = std::function<void()>;
// 定义时间戳类型，使用高精度时钟的时间点
using timestamp = std::chrono::high_resolution_clock::time_point;
// 定义毫秒类型
using ms = std::chrono::milliseconds;

// 定时器节点类，代表一个定时器任务
class TimerNode {
   public:
    TimerNode(size_t id, timestamp expires, timeOutCallBack cb) : id(id), expires(expires), cb(cb) {}
    // 定时器节点的唯一标识符
    size_t id;
    // 定时器的过期时间点
    timestamp expires;
    // 定时器到期时要执行的回调函数
    timeOutCallBack cb;
    // 重载小于运算符，用于比较两个定时器节点的过期时间
    bool operator<(TimerNode& t);
};

// 堆定时器类，使用堆结构管理定时器节点
class HeapTimer {
   public:
    // 构造函数，初始化堆定时器
    HeapTimer();
    // 析构函数，清理堆定时器资源
    ~HeapTimer();
    // 调整指定ID的定时器的过期时间
    void adjust(size_t id, size_t newexpires);
    // 添加一个新的定时器节点
    void addTimeNode(size_t id, size_t timeout, timeOutCallBack cb);
    // 执行指定ID的定时器任务
    void doWork(size_t id);
    // 清除所有定时器节点
    void clear();
    // 检查并执行过期的定时器任务
    void tick();
    // 移除堆顶的定时器节点
    void pop();
    // 获取下一个定时器任务的剩余时间
    int getNextTick();

   private:
    // 删除指定索引的定时器节点
    void del(size_t i);
    // 向上调整堆，确保堆的性质
    void siftUp(size_t i);
    // 向下调整堆，确保堆的性质
    bool siftDown(size_t i);
    // 交换两个索引对应的定时器节点
    void swapNode(size_t i, size_t j);

    // 存储定时器节点的堆
    std::vector<TimerNode> heap_timer;
    // 存储定时器ID到堆中索引的映射
    std::unordered_map<size_t, size_t> heap_ref;
};
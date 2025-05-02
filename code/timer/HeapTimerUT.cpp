#include "HeapTimer.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <thread>

// 使用标准命名空间
using namespace std::chrono_literals;
using namespace testing;

/**
 * HeapTimer测试类
 * 继承自Google Test的Test类，用于测试HeapTimer的功能
 */
class HeapTimerTest : public Test {
protected:
    // 每个测试用例执行前的设置
    virtual void SetUp() override {
        timer = std::make_unique<HeapTimer>();
        callback1_executed = false;
        callback2_executed = false;
        callback3_executed = false;
    }

    // 测试用例共享的成员变量
    std::unique_ptr<HeapTimer> timer;
    bool callback1_executed;
    bool callback2_executed;
    bool callback3_executed;
};

/**
 * 测试添加定时器功能
 * 验证添加定时器后，getNextTick()返回正确的值
 */
TEST_F(HeapTimerTest, AddTimerShouldScheduleCorrectly) {
    // 添加一个1000ms后执行的定时器
    timer->addTimeNode(1, 1000, [this]() { callback1_executed = true; });
    
    // 验证getNextTick返回的值不为-1（表示有定时器在等待）
    int next_tick = timer->getNextTick();
    ASSERT_NE(next_tick, -1);
    ASSERT_LE(next_tick, 1000);
}

/**
 * 测试tick触发过期回调功能
 * 验证当定时器过期时，tick()方法能正确触发回调函数
 */
TEST_F(HeapTimerTest, TickShouldTriggerExpiredCallbacks) {
    // 添加一个10ms后执行的定时器
    timer->addTimeNode(1, 10, [this]() { callback1_executed = true; });
    
    // 等待15ms确保定时器过期
    std::this_thread::sleep_for(15ms);
    
    // 调用tick()触发过期的定时器
    timer->tick();
    
    // 验证回调函数被执行
    ASSERT_TRUE(callback1_executed);
}

/**
 * 测试调整定时器功能
 * 验证adjust()方法能正确修改定时器的过期时间
 */
TEST_F(HeapTimerTest, AdjustShouldModifyTimer) {
    // 添加一个300ms后执行的定时器
    timer->addTimeNode(3, 300, [this]() { callback3_executed = true; });
    
    // 调整为50ms后执行
    timer->adjust(3, 50);
    
    // 获取下一个定时器的剩余时间
    int remaining = timer->getNextTick();
    
    // 验证剩余时间在合理范围内
    ASSERT_GT(remaining, 0);
    ASSERT_LE(remaining, 50);
}

/**
 * 测试立即执行定时器功能
 * 验证doWork()方法能立即执行指定ID的定时器任务
 */
TEST_F(HeapTimerTest, DoWorkShouldExecuteImmediately) {
    // 添加一个2000ms后执行的定时器
    timer->addTimeNode(2, 2000, [this]() { callback2_executed = true; });
    
    // 立即执行该定时器
    timer->doWork(2);
    
    // 验证回调函数被执行
    ASSERT_TRUE(callback2_executed);
    
    // 验证定时器被移除（getNextTick返回-1表示没有定时器）
    ASSERT_EQ(timer->getNextTick(), -1);
}

/**
 * 测试清除所有定时器功能
 * 验证clear()方法能正确清除所有定时器
 */
TEST_F(HeapTimerTest, ClearShouldRemoveAllTimers) {
    // 添加一个定时器
    timer->addTimeNode(1, 100, []() {});
    
    // 清除所有定时器
    timer->clear();
    
    // 验证没有定时器存在
    ASSERT_EQ(timer->getNextTick(), -1);
}

/**
 * 测试多个定时器的优先级
 * 验证最小堆能正确处理多个定时器的优先级
 */
TEST_F(HeapTimerTest, MultipleTimersShouldMaintainPriority) {
    // 添加多个定时器，过期时间不同
    timer->addTimeNode(1, 300, [this]() { callback1_executed = true; });
    timer->addTimeNode(2, 100, [this]() { callback2_executed = true; });
    timer->addTimeNode(3, 200, [this]() { callback3_executed = true; });
    
    // 验证下一个触发的是最早过期的定时器
    int next = timer->getNextTick();
    ASSERT_LE(next, 100);
    
    // 等待并触发第一个定时器
    std::this_thread::sleep_for(110ms);
    timer->tick();
    
    // 验证只有第二个回调被执行
    ASSERT_TRUE(callback2_executed);
    ASSERT_FALSE(callback1_executed);
    ASSERT_FALSE(callback3_executed);
    
    // 验证下一个是第三个定时器
    next = timer->getNextTick();
    ASSERT_LE(next, 200);
}
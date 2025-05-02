#include "HeapTimer.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
using namespace std::chrono_literals;

class HeapTimerTest : public testing::Test {
  protected:
    virtual void SetUp() override {
        timer = std::make_unique<HeapTimer>();
        callback1_executed = false;
        callback2_executed = false;
        callback3_executed = false;
    }
    std::unique_ptr<HeapTimer> timer;
    bool callback1_executed;
    bool callback2_executed;
    bool callback3_executed;
};

TEST_F(HeapTimerTest, AddTimerShouldScheduleCorrectly) {
    timer->addTimeNode(1, 1000, [this]() -> void {
        callback1_executed = true;
    });
    int next_tick = timer->getNextTick();
    ASSERT_NE(next_tick, -1);
    ASSERT_LE(next_tick, 1000);
}

TEST_F(HeapTimerTest, TickShouldTriggerExpiredCallbacks) {
    timer->addTimeNode(1, 100, [this]() -> void { callback1_executed = true; });
    std::this_thread::sleep_for(150ms);
    timer->tick();
    ASSERT_TRUE(callback1_executed);
}
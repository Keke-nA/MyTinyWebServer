#include <iostream>
#include <thread>
#include "HeapTimer.hpp"

void testCallback1() {
    std::cout << "Callback 1 executed!" << std::endl;
}

void testCallback2() {
    std::cout << "Callback 2 executed!" << std::endl;
}

void testCallback3() {
    std::cout << "Callback 3 executed!" << std::endl;
}

int main() {
    HeapTimer timer;
    
    // 测试1: 添加定时器
    timer.addTimeNode(1, 1000, testCallback1);  // 1秒后执行
    timer.addTimeNode(2, 2000, testCallback2);  // 2秒后执行
    timer.addTimeNode(3, 3000, testCallback3);  // 3秒后执行
    
    std::cout << "Timers added, waiting..." << std::endl;
    
    // 测试2: 检查下一个定时器
    int next = timer.getNextTick();
    std::cout << "Next tick in: " << next << "ms" << std::endl;
    
    // 测试3: 等待一段时间后tick
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    timer.tick();  // 应该触发callback1
    
    // 测试4: 调整定时器
    timer.adjust(3, 500);  // 将id为3的定时器调整为500ms后
    
    // 测试5: 直接执行某个定时器
    timer.doWork(2);  // 立即执行callback2
    
    // 测试6: 再次检查
    next = timer.getNextTick();
    std::cout << "Next tick now in: " << next << "ms" << std::endl;
    
    // 测试7: 等待剩余定时器执行
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    timer.tick();  // 应该触发调整后的callback3
    
    // 测试8: 清除所有定时器
    timer.clear();
    std::cout << "All timers cleared" << std::endl;
    
    return 0;
}
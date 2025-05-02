#include "Log.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// 假设这是你的 BlockDeque 类，已经包含在 BlockDeque.hpp 中
//#include "BlockDeque.hpp"

int main() {
    // 初始化日志系统，设置日志级别为 1（info），日志文件路径为 "./logs"，后缀为 ".log"，最大队列容量为 100
    Log::instance().init(0, "./logs", ".log", 100);

    // 写入不同级别的日志信息
    Log::instance().write(0, "This is a debug message");
    Log::instance().write(1, "This is an info message");
    Log::instance().write(2, "This is a warning message");
    Log::instance().write(3, "This is an error message");

    // 等待一段时间，确保异步日志写入完成
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 关闭日志系统
    // 这里没有显式的关闭函数，析构函数会在程序结束时自动调用
    // 如果你有其他需要，可以在合适的地方调用析构相关的逻辑

    std::cout << "Log test completed. Check the log file for output." << std::endl;

    return 0;
}
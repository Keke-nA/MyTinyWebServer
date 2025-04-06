#pragma once
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <cassert>
#include <cstdarg>
#include <memory>
#include <string>
#include <thread>
#include "../buffer/buffer.hpp"
#include "BlockDeque.hpp"

// 日志类的定义
class Log {
   public:
    // 获取 Log 类的单例实例
    static Log& instance();
    // 日志刷新线程函数，用于异步日志写入时刷新日志到文件
    static void flushLogThread();
    // 初始化日志系统，指定日志级别、路径、后缀和最大队列容量
    void init(size_t level, const char* path = "./log", const char* suffix = ".log", int maxqueuecapacity = 1024);
    // 写入日志信息，根据指定的日志级别和格式化字符串
    void write(size_t level, const char* format, ...);
    // 刷新日志缓冲区，确保数据写入文件
    void flush();
    // 获取当前的日志级别
    size_t getLevel();
    // 设置日志级别
    void setLevel(size_t level);
    // 判断日志系统是否已经打开
    bool isOpen();

   private:
    // 私有构造函数，确保只能通过单例模式创建实例
    Log();
    // 析构函数，释放资源
    ~Log();
    // 根据日志级别添加日志标题
    void appendLogLevelTitle(size_t level);
    // 异步写入日志的函数
    void asyncWrite();
    // 定义日志路径的最大长度
    static constexpr int LOG_PATH_LEN = 256;
    // 定义日志文件名的最大长度
    static constexpr int LOG_NAME_LEN = 256;
    // 定义每个日志文件的最大行数
    static constexpr int MAX_LINES = 50000;

    // 日志文件的路径
    const char* log_path;
    // 日志文件的后缀
    const char* log_suffix;
    // 每个日志文件的最大行数
    int max_lines;
    // 当前日志文件的行数
    int line_count;
    // 记录当前日期
    int log_today;
    // 日志系统是否打开的标志
    bool is_open;
    // 日志缓冲区
    Buffer log_buff;
    // 当前的日志级别
    size_t log_level;
    // 是否使用异步写入的标志
    bool is_async;
    // 日志文件指针
    FILE* log_fp;
    // 用于异步写入的阻塞队列智能指针
    std::unique_ptr<BlockDeque<std::string>> log_deque;
    // 异步写入线程的智能指针
    std::unique_ptr<std::thread> log_wirte_thread;
    // 日志操作的互斥锁
    std::mutex log_mtx;
};

// 基础日志宏，根据日志级别和格式写入日志信息并刷新
#define LOG_BASE(level, format, ...)                     \
    do {                                                 \
        Log& log = Log::instance();                      \
        if (log.isOpen() && log.getLevel() <= level) { \
            log.write(level, format, ##__VA_ARGS__);    \
            log.flush();                                \
        }                                                \
    } while (0);
// 调试级别日志宏，调用基础日志宏写入调试日志
#define LOG_DEBUG(format, ...)             \
    do {                                   \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
// 信息级别日志宏，调用基础日志宏写入信息日志
#define LOG_INFO(format, ...)              \
    do {                                   \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
// 警告级别日志宏，调用基础日志宏写入警告日志
#define LOG_WARN(format, ...)              \
    do {                                   \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
// 错误级别日志宏，调用基础日志宏写入错误日志
#define LOG_ERROR(format, ...)             \
    do {                                   \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);
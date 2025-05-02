#include "Log.hpp"

// 获取单例实例
Log& Log::instance() {
    // 使用静态局部变量实现线程安全的单例模式
    static Log instance;
    return instance;
}

// 异步日志刷新线程函数
void Log::flushLogThread() {
    // 调用单例实例的异步写入方法
    Log::instance().asyncWrite();
}

// 初始化日志系统
void Log::init(
    size_t level, const char* path, const char* suffix, int maxqueuecapacity) {
    // 确保路径和后缀不为空
    assert(path != nullptr && suffix != nullptr);
    is_open = true;
    log_level = level;

    // 根据队列容量决定是否使用异步模式
    if (maxqueuecapacity > 0) {
        is_async = true;
        if (!log_deque) {
            // 创建阻塞队列和写入线程
            log_deque = std::make_unique<BlockDeque<std::string>>();
            log_write_thread = std::make_unique<std::thread>(flushLogThread);
        }
    } else {
        is_async = false;
    }

    line_count = 0;
    // 获取当前系统时间
    time_t timer = time(nullptr);
    tm* systime = localtime(&timer);
    tm t = *systime;
    log_path = path;
    log_suffix = suffix;

    // 构造日志文件名
    char filename[LOG_NAME_LEN]{0};
    snprintf(
        filename,
        LOG_NAME_LEN - 1,
        "%s/%04d_%02d_%02d%s",
        log_path,
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        log_suffix);
    log_today = t.tm_mday;

    {
        std::lock_guard<std::mutex> lock(log_mtx);
        log_buff.retrieveAll();
        // 关闭已打开的文件
        if (log_fp) {
            flush();
            fclose(log_fp);
        }
        // 打开新的日志文件
        log_fp = fopen(filename, "a");
        if (log_fp == nullptr) {
            // 如果目录不存在则创建
            mkdir(log_path, 0777);
            log_fp = fopen(filename, "a");
        }
        assert(log_fp != nullptr);
    }
}

// 写入日志信息
void Log::write(size_t level, const char* format, ...) {
    // 获取当前时间
    timeval now_time{0, 0};
    gettimeofday(&now_time, nullptr);
    time_t tsec = now_time.tv_sec;
    tm* systime = localtime(&tsec);
    tm t = *systime;
    va_list valist;

    // 检查是否需要创建新的日志文件（新的一天或达到最大行数）
    if (log_today != t.tm_mday
        || (line_count && (line_count % MAX_LINES == 0))) {
        char newfile[LOG_NAME_LEN];
        char tail[36]{0};
        // 构造日期字符串
        snprintf(
            tail,
            36,
            "%04d_%02d_%02d",
            t.tm_year + 1900,
            t.tm_mon + 1,
            t.tm_mday);

        if (log_today != t.tm_mday) {
            // 新的一天，创建新的日志文件
            snprintf(
                newfile,
                LOG_NAME_LEN - 72,
                "%s/%s%s",
                log_path,
                tail,
                log_suffix);
            log_today = t.tm_mday;
            line_count = 0;
        } else {
            // 当天日志行数超过限制，创建新的日志文件
            snprintf(
                newfile,
                LOG_NAME_LEN - 72,
                "%s/%s-%d%s",
                log_path,
                tail,
                (line_count / MAX_LINES),
                log_suffix);
        }

        {
            std::lock_guard<std::mutex> lock(log_mtx);
            // 刷新并关闭当前日志文件
            flush();
            fclose(log_fp);
            // 打开新的日志文件
            log_fp = fopen(newfile, "a");
            assert(log_fp != nullptr);
        }
    }

    {
        std::lock_guard<std::mutex> lock(log_mtx);
        line_count++;

        // 添加时间戳到日志缓冲区
        int n = snprintf(
            log_buff.beginWrite(),
            128,
            "%04d_%02d_%02d %02d:%02d:%02d.%06ld",
            t.tm_year + 1900,
            t.tm_mon + 1,
            t.tm_mday,
            t.tm_hour,
            t.tm_min,
            t.tm_sec,
            now_time.tv_usec);
        log_buff.hasWritten(n);

        // 添加日志级别标题
        appendLogLevelTitle(level);

        // 添加用户格式化的日志内容
        va_start(valist, format);
        int m = vsnprintf(
            log_buff.beginWrite(),
            log_buff.writeableBytes(),
            format,
            valist);
        va_end(valist);
        log_buff.hasWritten(m);

        // 添加换行符
        log_buff.append("\n", 1);

        // 根据配置选择异步或同步写入
        if (is_async && log_deque && !log_deque->full()) {
            // 异步写入：将日志内容推入队列
            log_deque->push_back(log_buff.retrieveAllToStr());
        } else {
            // 同步写入：直接写入文件
            fwrite(log_buff.peek(), 1, log_buff.readableBytes(), log_fp);
        }
        log_buff.retrieveAll();
    }
}

// 刷新日志缓冲区
void Log::flush() {
    // 异步模式下刷新队列
    if (is_async) {
        log_deque->flush();
    }
    // 刷新文件缓冲区
    fflush(log_fp);
}

// 获取当前日志级别
size_t Log::getLevel() {
    std::lock_guard<std::mutex> lock(log_mtx);
    return log_level;
}

// 设置日志级别
void Log::setLevel(size_t level) {
    std::lock_guard<std::mutex> lock(log_mtx);
    log_level = level;
}

// 判断日志系统是否已打开
bool Log::isOpen() {
    std::lock_guard<std::mutex> lock(log_mtx);
    return is_open;
}

// 构造函数
Log::Log()
    : line_count(0), is_async(false), log_write_thread(nullptr),
      log_deque(nullptr), log_today(0), log_fp(nullptr) {
    // 初始化成员变量
}

// 析构函数
Log::~Log() {
    // 等待异步写入线程完成所有任务
    if (log_write_thread && log_write_thread->joinable()) {
        // 确保队列中的所有日志都被写入
        while (!log_deque->empty()) {
            log_deque->flush();
        }
        // 关闭队列并等待线程结束
        log_deque->close();
        log_write_thread->join();
    }

    // 关闭日志文件
    if (log_fp) {
        std::lock_guard<std::mutex> lock(log_mtx);
        flush();
        fclose(log_fp);
    }
}

// 添加日志级别标题
void Log::appendLogLevelTitle(size_t level) {
    // 根据日志级别添加对应的标题
    switch (level) {
    case 0:
        log_buff.append("[debug] : ", 10);
        break;
    case 1:
        log_buff.append("[info] : ", 9);
        break;
    case 2:
        log_buff.append("[warn] : ", 9);
        break;
    case 3:
        log_buff.append("[error] : ", 10);
        break;
    default:
        log_buff.append("[info] : ", 9);
        break;
    }
}

// 异步写入日志
void Log::asyncWrite() {
    std::string str{""};
    // 循环从队列中取出日志并写入文件
    while (log_deque->pop(str)) {
        std::lock_guard<std::mutex> lock(log_mtx);
        fputs(str.c_str(), log_fp);
    }
}

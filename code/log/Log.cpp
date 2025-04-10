#include "Log.hpp"

// 获取 Log 类的单例实例
Log& Log::instance() {
    static Log instance;
    return instance;
}

// 日志刷新线程函数，用于异步日志写入时刷新日志到文件
void Log::flushLogThread() {
    Log::instance().asyncWrite();
}

// 初始化日志系统，指定日志级别、路径、后缀和最大队列容量
void Log::init(size_t level, const char* path, const char* suffix, int maxqueuecapacity) {
    assert(path != nullptr && suffix != nullptr);
    is_open = true;
    log_level = level;
    if (maxqueuecapacity > 0) {
        is_async = true;
        if (!log_deque) {
            log_deque = std::make_unique<BlockDeque<std::string>>();
            log_wirte_thread = std::make_unique<std::thread>(flushLogThread);
        }
    } else {
        is_async = false;
    }
    line_count = 0;
    time_t timer = time(nullptr);
    tm* systime = localtime(&timer);
    tm t = *systime;
    log_path = path;
    log_suffix = suffix;
    char filename[LOG_NAME_LEN]{0};
    snprintf(filename, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", log_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             log_suffix);
    log_today = t.tm_mday;
    {
        std::lock_guard<std::mutex> lock(log_mtx);
        log_buff.retrieveAll();
        if (log_fp) {
            flush();
            fclose(log_fp);
        }
        log_fp = fopen(filename, "a");
        if (log_fp == nullptr) {
            mkdir(log_path, 0777);
            log_fp = fopen(filename, "a");
        }
        assert(log_fp != nullptr);
    }
}

// 写入日志信息，根据指定的日志级别和格式化字符串
void Log::write(size_t level, const char* format, ...) {
    timeval now_time{0, 0};
    gettimeofday(&now_time, nullptr);
    time_t tsec = now_time.tv_sec;
    tm* systime = localtime(&tsec);
    tm t = *systime;
    va_list valist;
    if (log_today != t.tm_mday || (line_count && (line_count % MAX_LINES == 0))) {
        char newfile[LOG_NAME_LEN];
        char tail[36]{0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (log_today != t.tm_mday) {
            snprintf(newfile, LOG_NAME_LEN - 72, "%s/%s%s", log_path, tail, log_suffix);
            log_today = t.tm_mday;
            line_count = 0;
        } else {
            snprintf(newfile, LOG_NAME_LEN - 72, "%s/%s-%d%s", log_path, tail, (line_count / MAX_LINES), log_suffix);
        }
        {
            std::lock_guard<std::mutex> lock(log_mtx);
            flush();
            fclose(log_fp);
            log_fp = fopen(newfile, "a");
            assert(log_fp != nullptr);
        }
    }
    {
        std::lock_guard<std::mutex> lock(log_mtx);
        line_count++;

        int n = snprintf(log_buff.beginWrite(), 128, "%04d_%02d_%02d %02d:%02d:%02d.%06ld", t.tm_year + 1900,
                         t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now_time.tv_usec);
        log_buff.hasWritten(n);

        appendLogLevelTitle(level);
        va_start(valist, format);
        int m = vsnprintf(log_buff.beginWrite(), log_buff.writeableBytes(), format, valist);
        va_end(valist);
        log_buff.hasWritten(m);

        // log_buff.append("\n\0", 2);
        log_buff.append("\n", 1);

        if (is_async && log_deque && !log_deque->full()) {
            log_deque->push_back(log_buff.retrieveAllToStr());
        } else {
            // fputs(log_buff.peek(), log_fp);
            fwrite(log_buff.peek(), 1, log_buff.readableBytes(), log_fp);
        }
        log_buff.retrieveAll();
    }
}

// 刷新日志缓冲区，确保数据写入文件
void Log::flush() {
    if (is_async) {
        log_deque->flush();
    }
    fflush(log_fp);
}

// 获取当前的日志级别
size_t Log::getLevel() {
    std::lock_guard<std::mutex> lock(log_mtx);
    return log_level;
}

// 设置日志级别
void Log::setLevel(size_t level) {
    std::lock_guard<std::mutex> lock(log_mtx);
    log_level = level;
}

// 判断日志系统是否已经打开
bool Log::isOpen() {
    std::lock_guard<std::mutex> lock(log_mtx);
    return is_open;
}

// 私有构造函数，确保只能通过单例模式创建实例
Log::Log()
    : line_count(0), is_async(false), log_wirte_thread(nullptr), log_deque(nullptr), log_today(0), log_fp(nullptr) {}

// 析构函数，释放资源
Log::~Log() {
    if (log_wirte_thread && log_wirte_thread->joinable()) {
        while (!log_deque->empty()) {
            log_deque->flush();
        }
        log_deque->close();
        log_wirte_thread->join();
    }
    if (log_fp) {
        std::lock_guard<std::mutex> lock(log_mtx);
        flush();
        fclose(log_fp);
    }
}

// 根据日志级别添加日志标题
void Log::appendLogLevelTitle(size_t level) {
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

// 异步写入日志的函数
void Log::asyncWrite() {
    std::string str{""};
    while (log_deque->pop(str)) {
        std::lock_guard<std::mutex> lock(log_mtx);
        fputs(str.c_str(), log_fp);
    }
}

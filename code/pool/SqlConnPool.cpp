#include "SqlConnPool.hpp"
#include <iostream>

// 获取连接池单例
SqlConnPool& SqlConnPool::instance() {
    // 使用静态局部变量实现单例模式
    static SqlConnPool conn_pool;
    return conn_pool;
}

// 获取一个数据库连接
MYSQL* SqlConnPool::getConn() {
    std::unique_lock<std::mutex> lock(conn_mtx);
    // 等待直到连接池中有可用连接
    conn_cv.wait(lock, [this] { return !conn_que.empty(); });
    // 获取队列前端的连接
    auto conn = conn_que.front();
    conn_que.pop();
    // 更新连接计数
    use_conn++;
    free_conn--;
    return conn;
}

// 释放一个数据库连接
void SqlConnPool::freeConn(MYSQL* conn) {
    {
        std::lock_guard<std::mutex> lock(conn_mtx);
        // 将连接放回队列
        conn_que.emplace(conn);
        // 更新连接计数
        free_conn++;
        use_conn--;
    }
    // 通知等待连接的线程
    conn_cv.notify_one();
}

// 获取空闲连接数量
size_t SqlConnPool::getFreeConnCount() {
    return free_conn;
}

// 初始化连接池
void SqlConnPool::initConn(
    const char* host,
    size_t port,
    const char* user,
    const char* pwd,
    const char* dbname,
    size_t connsize) {
    size_t success_cnt = 0;
    // 创建指定数量的数据库连接
    for (int i = 0; i < connsize; i++) {
        // 初始化MySQL连接
        MYSQL* raw_conn = mysql_init(nullptr);
        if (!raw_conn) {
            std::cout << "initConn failed!" << std::endl;
            continue;
        }
        // 建立实际的数据库连接
        MYSQL* real_conn = mysql_real_connect(
            raw_conn,
            host,
            user,
            pwd,
            dbname,
            port,
            nullptr,
            0);
        if (!real_conn) {
            std::cout << "mysql_real_connect failed!" << std::endl;
            mysql_close(raw_conn);
            continue;
        }
        // 将成功建立的连接加入队列
        std::lock_guard<std::mutex> lock(conn_mtx);
        conn_que.emplace(raw_conn);
        success_cnt++;
    }
    // 更新连接池状态
    max_conn = success_cnt;
    free_conn = success_cnt;
}

// 关闭连接池
void SqlConnPool::closePool() {
    std::lock_guard<std::mutex> lock(conn_mtx);
    // 关闭并释放所有连接
    while (!conn_que.empty()) {
        auto conn = conn_que.front();
        conn_que.pop();
        mysql_close(conn);
    }
    // 结束MySQL库
    mysql_library_end();
}

// 构造函数，初始化成员变量
SqlConnPool::SqlConnPool() : use_conn(0), free_conn(0), max_conn(0) {
}

// 析构函数，确保关闭连接池
SqlConnPool::~SqlConnPool() {
    closePool();
}

#include "SqlConnPool.hpp"
#include <iostream>

SqlConnPool& SqlConnPool::instance() {
    static SqlConnPool conn_pool;
    return conn_pool;
}

MYSQL* SqlConnPool::getConn() {
    std::unique_lock<std::mutex> lock(conn_mtx);
    conn_cv.wait(lock, [this] { return !conn_que.empty(); });
    auto conn = conn_que.front();
    conn_que.pop();
    use_conn++;
    free_conn--;
    return conn;
}

void SqlConnPool::freeConn(MYSQL* conn) {
    {
        std::lock_guard<std::mutex> lock(conn_mtx);
        conn_que.emplace(conn);
        free_conn++;
        use_conn--;
    }
    conn_cv.notify_one();
}

size_t SqlConnPool::getFreeConnCount() {
    return free_conn;
}

void SqlConnPool::initConn(const char* host,
                           size_t port,
                           const char* user,
                           const char* pwd,
                           const char* dbname,
                           size_t connsize) {
    size_t success_cnt = 0;
    for (int i = 0; i < connsize; i++) {
        MYSQL* raw_conn = mysql_init(nullptr);
        if (!raw_conn) {
            std::cout << "initConn failed!" << std::endl;
            continue;
        }
        MYSQL* real_conn = mysql_real_connect(raw_conn, host, user, pwd, dbname, port, nullptr, 0);
        if (!real_conn) {
            std::cout << "mysql_real_connect failed!" << std::endl;
            mysql_close(raw_conn);
            continue;
        }
        std::lock_guard<std::mutex> lock(conn_mtx);
        conn_que.emplace(raw_conn);
        success_cnt++;
    }
    max_conn = success_cnt;
    free_conn = success_cnt;
}

void SqlConnPool::closePool() {
    std::lock_guard<std::mutex> lock(conn_mtx);
    while (!conn_que.empty()) {
        auto conn = conn_que.front();
        conn_que.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

SqlConnPool::SqlConnPool() : use_conn(0), free_conn(0), max_conn(0) {}

SqlConnPool::~SqlConnPool() {
    closePool();
}

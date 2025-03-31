#pragma once

#include <mysql/mysql.h>
#include <condition_variable>
#include <mutex>
#include <queue>

class SqlConnPool {
   public:
    static SqlConnPool& instance();
    MYSQL* getConn();
    void freeConn(MYSQL* conn);
    size_t getFreeConnCount();
    void initConn(const char* host,
                  size_t port,
                  const char* user,
                  const char* pwd,
                  const char* dbname,
                  size_t connsize);
    void closePool();

   private:
    SqlConnPool();
    ~SqlConnPool();

    size_t max_conn;
    size_t use_conn;
    size_t free_conn;

    std::queue<MYSQL*> conn_que;
    std::mutex conn_mtx;
    std::condition_variable conn_cv;
};
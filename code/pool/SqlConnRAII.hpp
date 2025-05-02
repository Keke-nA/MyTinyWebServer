#pragma once

#include "SqlConnPool.hpp"
#include <assert.h>

// 数据库连接RAII管理类
// 用于自动管理数据库连接的获取和释放
class SqlConnRAII {
  public:
    // 构造函数，获取一个数据库连接
    // sql: 输出参数，用于存储获取的连接
    // connpool: 连接池指针
    SqlConnRAII(MYSQL** sql, SqlConnPool* connpool) {
        assert(connpool);
        *sql = connpool->getConn();
        raii_sql = *sql;
        raii_connpoool = connpool;
    }

    // 析构函数，自动释放数据库连接
    ~SqlConnRAII() {
        if (raii_sql) {
            raii_connpoool->freeConn(raii_sql);
        }
    }

  private:
    MYSQL* raii_sql;             // 持有的数据库连接
    SqlConnPool* raii_connpoool; // 连接池指针
};
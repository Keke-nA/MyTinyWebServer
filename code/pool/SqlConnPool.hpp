#pragma once

#include <condition_variable>
#include <mutex>
#include <mysql/mysql.h>
#include <queue>

// MySQL连接池类
// 实现了数据库连接的管理，包括连接的创建、获取和释放
class SqlConnPool {
  public:
    // 获取连接池单例
    // 返回连接池单例引用
    static SqlConnPool& instance();

    // 获取一个数据库连接
    // 返回MYSQL*数据库连接指针
    MYSQL* getConn();

    // 释放一个数据库连接
    // conn: 要释放的连接指针
    void freeConn(MYSQL* conn);

    // 获取空闲连接数量
    // 返回空闲连接数
    size_t getFreeConnCount();

    // 初始化连接池
    // host: 数据库主机地址
    // port: 数据库端口
    // user: 用户名
    // pwd: 密码
    // dbname: 数据库名
    // connsize: 连接池大小
    void initConn(
        const char* host,
        size_t port,
        const char* user,
        const char* pwd,
        const char* dbname,
        size_t connsize);

    // 关闭连接池
    void closePool();

  private:
    SqlConnPool();
    ~SqlConnPool();

    size_t max_conn;  // 最大连接数
    size_t use_conn;  // 已使用连接数
    size_t free_conn; // 空闲连接数

    std::queue<MYSQL*> conn_que;     // 连接队列
    std::mutex conn_mtx;             // 互斥锁
    std::condition_variable conn_cv; // 条件变量
};
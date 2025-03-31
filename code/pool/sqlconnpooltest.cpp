#include "SqlConnPool.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <string>

std::mutex cout_mutex;

void testThread(int threadId) {
    MYSQL* conn = SqlConnPool::instance().getConn();
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Thread " << threadId << " acquired connection" << std::endl;
    }

    if (conn) {
        // 构造插入语句
        std::string query = "INSERT INTO user (username, password) VALUES ('test_user_" + 
                           std::to_string(threadId) + "', 'password_" + 
                           std::to_string(threadId) + "')";

        if (mysql_query(conn, query.c_str())) {
            std::cerr << "Thread " << threadId << " query failed: " << mysql_error(conn) << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Thread " << threadId << " inserted successfully!" << std::endl;
        }

        SqlConnPool::instance().freeConn(conn);
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Thread " << threadId << " released connection" << std::endl;
        }
    } else {
        std::cerr << "Thread " << threadId << " failed to get connection" << std::endl;
    }
}

int main() {
    // 初始化连接池（确保参数正确）
    const char* host = "localhost";
    size_t port = 3306;
    const char* user = "root";
    const char* pwd = "123456";
    const char* dbname = "webserver"; // 确保该数据库存在
    size_t connsize = 5;

    std::cout << "Initializing connection pool..." << std::endl;
    SqlConnPool::instance().initConn(host, port, user, pwd, dbname, connsize);

    // 启动 10 个线程
    std::vector<std::thread> threads;
    const int threadCount = 10;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(testThread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Closing connection pool..." << std::endl;
    SqlConnPool::instance().closePool();

    return 0;
}
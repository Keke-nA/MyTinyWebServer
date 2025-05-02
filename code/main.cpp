#include "server/WebServer.hpp"
#include <unistd.h>

int main() {
    // 创建WebServer对象，传入各项初始化参数
    WebServer server(
        5005,        // 监听端口
        3,           // 触发模式（ET/LT）
        60000,       // 超时时间（毫秒）
        false,       // 是否开启优雅关闭
        3306,        // 数据库端口
        "root",      // 数据库用户名
        "root",      // 数据库密码
        "webserver", // 数据库名
        12,          // 数据库连接池数量
        10,          // 线程池线程数量
        true,        // 是否开启日志
        1,           // 日志等级
        1024         // 日志队列容量
    );
    // 启动服务器主循环
    server.start();
    return 0;
}
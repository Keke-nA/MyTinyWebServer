#pragma once

#include "../http/HttpConn.hpp"
#include "../pool/threadpool.hpp"
#include "../timer/HeapTimer.hpp"
#include "Epoller.hpp"
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

// WebServer类：负责整个Web服务器的初始化、运行和资源管理
class WebServer {
  public:
    // 构造函数：初始化服务器各项参数和资源
    WebServer(
        int port,
        int trigmode,
        int timeoutms,
        bool optlinger,
        int sqlport,
        const char* sqluser,
        const char* sqlpwd,
        const char* dbname,
        int connpollnum,
        int threadnum,
        bool openlog,
        int loglevel,
        int logquesize);

    // 析构函数：释放所有资源
    ~WebServer();

    // 启动服务器主循环
    void start();

  private:
    // 初始化监听套接字
    bool initSocket();
    // 初始化epoll事件触发模式
    void initEventMode(int trigmode);
    // 添加新客户端连接
    void addClient(int fd, sockaddr_in addr);
    // 处理监听套接字上的新连接
    void dealListen();
    // 处理写事件，将写任务交给线程池
    void dealWrite(HttpConn* client);
    // 处理读事件，将读任务交给线程池
    void dealRead(HttpConn* client);
    // 发送错误信息给客户端
    void sendError(int fd, const char* info);
    // 延长连接的超时时间
    void extentTime(HttpConn* client);
    // 关闭客户端连接
    void closeConn(HttpConn* client);
    // 线程池中处理读事件的回调
    void onRead(HttpConn* client);
    // 线程池中处理写事件的回调
    void onWrite(HttpConn* client);
    // 处理HTTP请求的回调
    void onProcess(HttpConn* client);
    // 设置文件描述符为非阻塞
    static int setFdNonBlock(int fd);
    // 支持的最大客户端连接数
    static const int MAX_FD = 65536;
    // 服务器监听端口号
    int ws_port;
    // 是否开启优雅关闭
    bool open_linger;
    // 连接超时时间（毫秒）
    int timeout_ms;
    // 服务器是否关闭
    bool is_close;
    // 监听套接字文件描述符
    int listen_fd;
    // 静态资源目录路径
    char* src_dir;
    // 监听事件类型（ET/LT等）
    size_t listen_event;
    // 连接事件类型（ET/LT/ONESHOT等）
    size_t conn_event;
    // 定时器，用于管理连接超时
    std::unique_ptr<HeapTimer> heap_timer;
    // 线程池，用于处理业务逻辑
    std::unique_ptr<ThreadPool> thread_pool;
    // epoll实例，用于事件驱动
    std::unique_ptr<Epoller> epoller;
    // 保存所有客户端连接的映射表
    std::unordered_map<int, HttpConn> users;
};
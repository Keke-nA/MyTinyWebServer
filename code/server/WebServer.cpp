#include "WebServer.hpp"  // 假设头文件名为 WebServer.hpp

using namespace std;

// 构造函数实现框架（使用成员初始化列表）
WebServer::WebServer(int port,
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
                     int logquesize)
    : ws_port(port),
      open_linger(optlinger),
      timeout_ms(timeoutms),
      is_close(false),
      heap_timer(make_unique<HeapTimer>()),
      thread_pool(make_unique<ThreadPool>(threadnum)),
      epoller(make_unique<Epoller>()) {
    // 构造函数逻辑框架（具体实现待补充）
    // 示例：初始化资源目录、数据库连接池、日志系统等
    /*src_dir = getcwd(nullptr, 256);
    assert(src_dir);
    strncat(src_dir, "../../resources/", 17);*/
    // const char* path = "/home/gao/code/MyTinyWebServer/resources/";
    // strcpy(src_dir, path);
    const char* basePath = "/home/gao/code/MyTinyWebServer/resources";
    src_dir = new char[strlen(basePath) + 1];  // +1 用于存放字符串结束符 '\0'
    strcpy(src_dir, basePath);                 // 深拷贝字符串
    //std::cout << src_dir << std::endl;
    HttpConn::user_count = 0;
    HttpConn::src_dir = src_dir;
    SqlConnPool::instance().initConn("localhost", sqlport, sqluser, sqlpwd, dbname, connpollnum);
    initEventMode(trigmode);
    if (!initSocket()) {
        is_close = true;
    }
    if (openlog) {
        Log::instance().init(loglevel, "./log", ".log", logquesize);
    }
    if (is_close) {
        LOG_ERROR("WebServer.cpp: 48     ==========Server init error==========");
    } else {
        LOG_INFO("WebServer.cpp: 50     ==========Server init==========");
        LOG_INFO("WebServer.cpp: 51     Port: %d, OpenLinger: %s", port, optlinger ? true : false);
        LOG_INFO("WebServer.cpp: 52     Listen Mode: %s, OpenConn Mode: %s", (listen_event & EPOLLET ? "ET" : "LT"),
                 (conn_event & EPOLLET ? "ET" : "LT"));
        LOG_INFO("WebServer.cpp: 54     LogSys level: %d", loglevel);
        LOG_INFO("WebServer.cpp: 55     srcdir: %s", HttpConn::src_dir);
        LOG_INFO("WebServer.cpp: 56     SqlConnPool num: %d, ThreadPool num: %d", connpollnum, threadnum);
    }
}

// 析构函数实现框架
WebServer::~WebServer() {
    // 析构函数逻辑框架（释放资源）
    if (listen_fd > 0) {
        close(listen_fd);
    }
    is_close = true;
    if (src_dir) {
        free(src_dir);
    }
    SqlConnPool::instance().closePool();
    // 示例：关闭数据库连接池、清理定时器等
}

void WebServer::start() {
    int timems = -1;
    if (!is_close) {
        LOG_INFO("WebServer.cpp: 77     ==========Server start==========");
    }
    while (!is_close) {
        if (timeout_ms > 0) {
            timems = heap_timer->getNextTick();
        }
        //std::cout << "WebServer:83" << std::endl;
        int eventcnt = epoller->wait(timems);
        //std::cout << "WebServer:85  " << eventcnt << std::endl;
        // std::cout << "WebServer::start() eventcnt:" << eventcnt << std::endl;
        for (int i = 0; i < eventcnt; i++) {
            int fd = epoller->getEventFd(i);
            size_t events = epoller->getEvents(i);
            if (fd == listen_fd) {
                //std::cout << "WebServer:91" << std::endl;
                dealListen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users.count(fd) > 0);
                //std::cout << "WebServer.cpp:92  " << events << " " << EPOLLRDHUP << " " << EPOLLHUP << " " << EPOLLERR
                 //         << std::endl;
                //std::cout << "Closing connection for fd: " << fd << std::endl;
                closeConn(&users[fd]);
            } else if (events & EPOLLIN) {
                assert(users.count(fd) > 0);
                dealRead(&users[fd]);
            } else if (events & EPOLLOUT) {
                assert(users.count(fd) > 0);
                dealWrite(&users[fd]);
            } else {
                LOG_ERROR("WebServer.cpp: 100     Unexpected event!");
            }
        }
    }
}

// 初始化监听套接字（私有成员函数）
bool WebServer::initSocket() {
    // 实现框架（待补充：socket()/bind()/listen() 等系统调用）
    int ret;
    struct sockaddr_in addr;
    if (ws_port > 65535 || ws_port < 1024) {
        LOG_ERROR("Port: %d error!", ws_port);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ws_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    struct linger optlinger{};
    if (open_linger) {
        optlinger.l_onoff = 1;
        optlinger.l_linger = 1;
    }
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("WebServer.cpp: 125     Create socket error! port: %d", ws_port);
        return false;
    }
    ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optlinger, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("WebServer.cpp: 130     set socket setsockopt error!");
        close(listen_fd);
        return false;
    }
    ret = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("WebServer.cpp: 136     Bind Port: %d error!", ws_port);
        close(listen_fd);
        return false;
    }
    ret = listen(listen_fd, 6);
    if (ret < 0) {
        LOG_ERROR("WebServer.cpp: 142     Listen port: %d error!", ws_port);
        close(listen_fd);
        return false;
    }
    ret = epoller->addFd(listen_fd, listen_event | EPOLLIN);
    if (ret < 0) {
        LOG_ERROR("WebServer.cpp: 148     Add listen fd to epoll error!");
        close(listen_fd);
        return false;
    }
    setFdNonBlock(listen_fd);
    LOG_INFO("WebServer.cpp: 153     Server Port: %d", ws_port);
    return true;
}

// 初始化事件触发模式（私有成员函数）
void WebServer::initEventMode(int trigmode) {
    // 实现框架（待补充：设置 EPOLLET/LT 模式、EPOLLONESHOT 等）
    listen_event = EPOLLRDHUP;
    conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigmode) {
        case 0:
            break;
        case 1:
            conn_event |= EPOLLET;
            break;
        case 2:
            listen_event |= EPOLLET;
            break;
        case 3:
            listen_event |= EPOLLET;
            conn_event |= EPOLLET;
            break;
        default:
            listen_event |= EPOLLET;
            conn_event |= EPOLLET;
            break;
    }
    HttpConn::is_et = (conn_event & EPOLLET);
}

// 添加新客户端连接（私有成员函数）
void WebServer::addClient(int fd, sockaddr_in addr) {
    // 实现框架（待补充：初始化 HttpConn 对象、注册到 epoll 和定时器）
    assert(fd > 0);
    users[fd].httpcnInit(fd, addr);
    if (timeout_ms > 0) {
        //std::cout << "WebServer.cpp : 189  " << timeout_ms << std::endl;
        heap_timer->addTimeNode(fd, timeout_ms, std::bind(&WebServer::closeConn, this, &users[fd]));
    }
    epoller->addFd(fd, EPOLLIN | conn_event);
    setFdNonBlock(fd);
    LOG_INFO("WebServer.cpp: 193     Client[%d] in", users[fd].getFd());
}

// 处理监听套接字事件（私有成员函数）
void WebServer::dealListen() {
    // 实现框架（待补充：accept() 新连接、处理 ET 模式下的批量读取）
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listen_fd, (struct sockaddr*)&addr, &len);
        if (fd < 0) {
            return;
        }
        if (HttpConn::user_count >= MAX_FD) {
            sendError(fd, "Server busy!");
            LOG_WARN("WebServer.cpp: 208     Client is full!");
            return;
        }
        addClient(fd, addr);
    } while (listen_event & EPOLLET);
}

// 处理写事件（私有成员函数）
void WebServer::dealWrite(HttpConn* client) {
    // 实现框架（待补充：将写任务提交到线程池）
    assert(client);
    extentTime(client);
    thread_pool->addTask(std::bind(&WebServer::onWrite, this, client));
}

// 处理读事件（私有成员函数）
void WebServer::dealRead(HttpConn* client) {
    // 实现框架（待补充：将读任务提交到线程池）
    assert(client);
    extentTime(client);
    thread_pool->addTask(std::bind(&WebServer::onRead, this, client));
}

// 发送错误信息（私有成员函数）
void WebServer::sendError(int fd, const char* info) {
    // 实现框架（待补充：send() 错误信息并关闭连接）
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("WebServer.cpp: 237     send error to client[%d] error!", fd);
    }
    close(fd);
}

// 延长连接超时时间（私有成员函数）
void WebServer::extentTime(HttpConn* client) {
    // 实现框架（待补充：调整堆定时器中的超时时间）
    assert(client);
    if (timeout_ms > 0) {
        heap_timer->adjust(client->getFd(), timeout_ms);
    }
}

// 关闭客户端连接（私有成员函数）
void WebServer::closeConn(HttpConn* client) {
    // 实现框架（待补充：从 epoll/定时器中移除连接，释放资源）
    assert(client);
    int fd = client->getFd();
    //std::cout << "Closing connection for user with fd: " << fd << std::endl;
    LOG_INFO("WebServer.cpp: 256     Client[%d] quit!", fd);
    epoller->delDf(fd);
    client->httpcnClose();
}

// 处理读事件逻辑（私有成员函数）
void WebServer::onRead(HttpConn* client) {
    // 实现框架（待补充：调用 HttpConn::read() 读取数据）
    assert(client);
    int ret = -1;
    int readerror = 0;
    ret = client->httpcnRead(&readerror);
    if (ret <= 0 && readerror != EAGAIN) {
        //std::cout << "WebServer.cpp : 270  " << ret << "  " << readerror << std::endl;
        closeConn(client);
        return;
    }
    onProcess(client);
}

// 处理写事件逻辑（私有成员函数）
void WebServer::onWrite(HttpConn* client) {
    // 实现框架（待补充：调用 HttpConn::write() 发送数据）
    assert(client);
    int ret = -1;
    int writeerror = 0;
    ret = client->httpcnWrite(&writeerror);
    if (client->toWriteBytes() == 0) {
        if (client->isKeepAlive()) {
            onProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeerror == EAGAIN) {
            epoller->modFd(client->getFd(), conn_event | EPOLLOUT);
            return;
        }
    }
    //std::cout << "WebServer.cpp: 295  " << ret << "  " << writeerror << std::endl;
    closeConn(client);
}

// 处理 HTTP 请求逻辑（私有成员函数）
void WebServer::onProcess(HttpConn* client) {
    // 实现框架（待补充：调用 HttpConn::process() 解析请求并生成响应）
    if (client->process()) {
        epoller->modFd(client->getFd(), conn_event | EPOLLOUT);
    } else {
        epoller->modFd(client->getFd(), conn_event | EPOLLIN);
    }
}

// 设置文件描述符为非阻塞模式（静态成员函数）
int WebServer::setFdNonBlock(int fd) {
    // 实现框架（待补充：使用 fcntl() 设置 O_NONBLOCK 标志）
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
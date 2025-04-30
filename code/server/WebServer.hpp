#pragma once

#include "../http/HttpConn.hpp"
#include "../log/Log.hpp"
#include "../pool/SqlConnPool.hpp"
#include "../pool/SqlConnRAII.hpp"
#include "../pool/threadpool.hpp"
#include "../timer/HeapTimer.hpp"
#include "Epoller.hpp"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

class WebServer {
  public:
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
    ~WebServer();
    void start();

  private:
    bool initSocket();
    void initEventMode(int trigmode);
    void addClient(int fd, sockaddr_in addr);
    void dealListen();
    void dealWrite(HttpConn* client);
    void dealRead(HttpConn* client);
    void sendError(int fd, const char* info);
    void extentTime(HttpConn* client);
    void closeConn(HttpConn* client);
    void onRead(HttpConn* client);
    void onWrite(HttpConn* client);
    void onProcess(HttpConn* client);
    static int setFdNonBlock(int fd);
    static const int MAX_FD = 65536;
    int ws_port;
    bool open_linger;
    int timeout_ms;
    bool is_close;
    int listen_fd;
    char* src_dir;
    size_t listen_event;
    size_t conn_event;
    std::unique_ptr<HeapTimer> heap_timer;
    std::unique_ptr<ThreadPool> thread_pool;
    std::unique_ptr<Epoller> epoller;
    std::unordered_map<int, HttpConn> users;
};
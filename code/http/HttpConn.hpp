#pragma once
#include "../buffer/Buffer.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <arpa/inet.h>
#include <assert.h>
#include <atomic>
#include <cstdlib>
#include <sys/uio.h>

class HttpConn {
  public:
    HttpConn();
    ~HttpConn();

    // 初始化HTTP连接
    void httpcnInit(int sockfd, const sockaddr_in& addr);
    // 读取HTTP请求数据
    ssize_t httpcnRead(int* saveerrno);
    // 写入HTTP响应数据
    ssize_t httpcnWrite(int* saveerror);
    // 关闭HTTP连接
    void httpcnClose();
    // 获取文件描述符
    int getFd() const;
    // 获取端口号
    int getPort() const;
    // 获取IP地址
    const char* getIp() const;
    // 获取地址结构
    sockaddr_in getAddr() const;
    // 处理HTTP请求
    bool process();
    // 获取待写入的字节数
    int toWriteBytes();
    // 判断是否为长连接
    bool isKeepAlive() const;

    static bool is_et;                  // 是否使用ET模式
    static const char* src_dir;         // 资源目录
    static std::atomic<int> user_count; // 用户计数

  private:
    int httpcn_fd;                  // 连接的文件描述符
    struct sockaddr_in httpcn_addr; // 客户端地址
    bool httpcn_isclose;            // 连接是否关闭
    int httpcn_iocnt;               // IO向量计数
    struct iovec httpcn_iovec[2];   // IO向量
    Buffer httpcn_read_buff;        // 读缓冲区
    Buffer httpcn_write_buff;       // 写缓冲区
    HttpRequest httpcn_request;     // HTTP请求对象
    HttpResponse httpcn_response;   // HTTP响应对象
};
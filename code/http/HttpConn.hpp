#pragma once
#include <arpa/inet.h>
#include <assert.h>
#include <sys/uio.h>
#include <cstdlib>
#include "../buffer/Buffer.hpp"
#include "../log/Log.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class HttpConn {
   public:
    HttpConn();
    ~HttpConn();
    void httpcnInit(int sockfd, const sockaddr_in& addr);
    ssize_t httpcnRead(int* saveerrno);
    ssize_t httpcnWrite(int* saveerror);
    void httpcnClose();
    int getFd() const;
    int getPort() const;
    const char* getIp() const;
    sockaddr_in getAddr() const;
    bool process();
    int toWriteBytes();
    bool isKeepAlive() const;
    static bool is_et;
    static const char* src_dir;
    static std::atomic<int> user_count;

   private:
    int httpcn_fd;
    struct sockaddr_in httpcn_addr;
    bool httpcn_isclose;
    int httpcn_iocnt;
    struct iovec httpcn_iovec[2];
    Buffer httpcn_read_buff;
    Buffer httpcn_write_buff;
    HttpRequest httpcn_request;
    HttpResponse httpcn_response;
};
#include "HttpConn.hpp"

// 静态成员变量初始化
bool HttpConn::is_et;
const char* HttpConn::src_dir;
std::atomic<int> HttpConn::user_count;

// 构造函数
HttpConn::HttpConn() : httpcn_fd(-1), httpcn_addr{}, httpcn_isclose(true), httpcn_iocnt(0), httpcn_iovec{} {}

// 析构函数
HttpConn::~HttpConn() {
    // 可以在这里进行一些资源释放操作
    httpcnClose();
}

// 初始化函数
void HttpConn::httpcnInit(int sockfd, const sockaddr_in& addr) {
    // 实现初始化逻辑
    assert(sockfd > 0);
    httpcn_fd = sockfd;
    httpcn_addr = addr;
    user_count++;
    httpcn_read_buff.retrieveAll();
    httpcn_write_buff.retrieveAll();
    httpcn_isclose = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", httpcn_fd, getIp(), getPort(), static_cast<int>(user_count));
}

// 读取数据函数
ssize_t HttpConn::httpcnRead(int* saveerrno) {
    // 实现读取数据的逻辑
    ssize_t len = -1;
    do {
        len = httpcn_read_buff.readFd(httpcn_fd, saveerrno);
        if (len <= 0) {
            break;
        }
    } while (is_et);
    return len;
}

// 写入数据函数
ssize_t HttpConn::httpcnWrite(int* saveerror) {
    // 实现写入数据的逻辑
    ssize_t len = -1;
    do {
        len = writev(httpcn_fd, httpcn_iovec, httpcn_iocnt);
        if (len <= 0) {
            *saveerror = errno;
            break;
        }
        if (httpcn_iovec[0].iov_len + httpcn_iovec[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > httpcn_iovec[0].iov_len) {
            httpcn_iovec[1].iov_base = (uint8_t*)httpcn_iovec[1].iov_base + (len - httpcn_iovec[0].iov_len);
            httpcn_iovec[1].iov_len -= (len - httpcn_iovec[0].iov_len);
            httpcn_write_buff.retrieveAll();
            httpcn_iovec[0].iov_len = 0;
        } else {
            httpcn_iovec[0].iov_base = (uint8_t*)httpcn_iovec[0].iov_base + len;
            httpcn_iovec[0].iov_len -= len;
            httpcn_write_buff.retrieve(len);
        }
    } while (is_et || toWriteBytes() > 10240);
}

// 关闭连接函数
void HttpConn::httpcnClose() {
    // 实现关闭连接的逻辑
    httpcn_response.unMapfile();
    if (!httpcn_isclose) {
        httpcn_isclose = true;
        user_count--;
        close(httpcn_fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", httpcn_fd, getIp(), getPort(), static_cast<int>(user_count));
    }
}

// 获取套接字描述符函数
int HttpConn::getFd() const {
    // 实现获取套接字描述符的逻辑
    return httpcn_fd;
}

// 获取端口号函数
int HttpConn::getPort() const {
    // 实现获取端口号的逻辑
    return ntohs(httpcn_addr.sin_port);
}

// 获取 IP 地址函数
const char* HttpConn::getIp() const {
    // 实现获取 IP 地址的逻辑
    return inet_ntoa(httpcn_addr.sin_addr);
}

// 获取地址结构体函数
sockaddr_in HttpConn::getAddr() const {
    // 实现获取地址结构体的逻辑
    return httpcn_addr;
}

// 处理请求函数
bool HttpConn::process() {
    // 实现处理请求的逻辑
    httpcn_request.initHttprq();
    if (httpcn_read_buff.readableBytes() <= 0) {
        return false;
    } else if (httpcn_request.parse(httpcn_read_buff)) {
        LOG_DEBUG("%s", httpcn_request.path().c_str());
        httpcn_response.res_init(src_dir, httpcn_request.path(), httpcn_request.isKeepAlive(), 200);
    } else {
        httpcn_response.res_init(src_dir, httpcn_request.path(), false, 400);
    }
    httpcn_response.makeResponse(httpcn_write_buff);
    httpcn_iovec[0].iov_base = const_cast<char*>(httpcn_write_buff.peek());
    httpcn_iovec[0].iov_len = httpcn_write_buff.readableBytes();
    httpcn_iocnt = 1;

    if (httpcn_response.fileLen() > 0 && httpcn_response.mmapFile()) {
        httpcn_iovec[1].iov_base = httpcn_response.mmapFile();
        httpcn_iovec[1].iov_len = httpcn_response.fileLen();
        httpcn_iocnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d to %d", httpcn_response.fileLen(), httpcn_iocnt, toWriteBytes());
    return true;
}

// 获取待写入字节数函数
int HttpConn::toWriteBytes() {
    // 实现获取待写入字节数的逻辑
    return httpcn_iovec[0].iov_len + httpcn_iovec[1].iov_len;
}

// 判断是否为长连接函数
bool HttpConn::isKeepAlive() const {
    // 实现判断是否为长连接的逻辑
    return httpcn_request.isKeepAlive();
}
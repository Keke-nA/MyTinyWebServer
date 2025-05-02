#include "HttpConn.hpp"
#include "../log/Log.hpp"

// 静态成员变量初始化
bool HttpConn::is_et;
const char* HttpConn::src_dir;
std::atomic<int> HttpConn::user_count;

// 构造函数
HttpConn::HttpConn()
    : httpcn_fd(-1), httpcn_addr{}, httpcn_isclose(true), httpcn_iocnt(0),
      httpcn_iovec{} {
}

// 析构函数
HttpConn::~HttpConn() {
    httpcnClose();
}

// 初始化连接
void HttpConn::httpcnInit(int sockfd, const sockaddr_in& addr) {
    // 确保套接字描述符有效
    assert(sockfd > 0);
    // 保存连接信息
    httpcn_fd = sockfd;
    httpcn_addr = addr;
    // 增加用户计数
    user_count++;
    // 清空读写缓冲区
    httpcn_read_buff.retrieveAll();
    httpcn_write_buff.retrieveAll();
    // 设置连接为开启状态
    httpcn_isclose = false;
    // 记录连接信息到日志
    LOG_INFO(
        "HttpConn.cpp: 27     Client[%d](%s:%d) in, userCount:%d",
        httpcn_fd,
        getIp(),
        getPort(),
        static_cast<int>(user_count));
}

// 读取客户端数据
ssize_t HttpConn::httpcnRead(int* saveerrno) {
    ssize_t len = -1;
    do {
        // 从套接字读取数据到缓冲区
        len = httpcn_read_buff.readFd(httpcn_fd, saveerrno);
        if (len <= 0) {
            // 读取失败或无数据可读，退出循环
            break;
        }
    } while (is_et); // 在ET模式下需要一次性读取所有数据
    return len;
}

// 向客户端写入数据
ssize_t HttpConn::httpcnWrite(int* saveerror) {
    ssize_t len = -1;
    do {
        // 使用writev一次性写入多个缓冲区的数据
        len = writev(httpcn_fd, httpcn_iovec, httpcn_iocnt);
        if (len <= 0) {
            // 写入失败，保存错误码并退出
            *saveerror = errno;
            break;
        }

        // 检查是否所有数据都已写入
        if (httpcn_iovec[0].iov_len + httpcn_iovec[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > httpcn_iovec[0].iov_len) {
            // 第一个缓冲区已全部写入，调整第二个缓冲区的指针和长度
            httpcn_iovec[1].iov_base = (uint8_t*)httpcn_iovec[1].iov_base
                                       + (len - httpcn_iovec[0].iov_len);
            httpcn_iovec[1].iov_len -= (len - httpcn_iovec[0].iov_len);
            // 清空第一个缓冲区
            httpcn_write_buff.retrieveAll();
            httpcn_iovec[0].iov_len = 0;
        } else {
            // 只写入了第一个缓冲区的部分数据，调整指针和长度
            httpcn_iovec[0].iov_base = (uint8_t*)httpcn_iovec[0].iov_base + len;
            httpcn_iovec[0].iov_len -= len;
            httpcn_write_buff.retrieve(len);
        }
    } while (is_et || toWriteBytes() > 10240); // ET模式或剩余数据量大时继续写入
    return len;
}

// 关闭连接
void HttpConn::httpcnClose() {
    // 解除文件映射
    httpcn_response.unMapfile();
    // 检查连接是否已关闭
    if (!httpcn_isclose) {
        httpcn_isclose = true;
        // 减少用户计数
        user_count--;
        // 关闭套接字
        close(httpcn_fd);
        // 记录关闭信息到日志
        LOG_INFO(
            "HttpConn.cpp: 77     Client[%d](%s:%d) quit, UserCount:%d",
            httpcn_fd,
            getIp(),
            getPort(),
            static_cast<int>(user_count));
    }
}

// 获取套接字描述符
int HttpConn::getFd() const {
    return httpcn_fd;
}

// 获取客户端端口号
int HttpConn::getPort() const {
    // 将网络字节序转换为主机字节序
    return ntohs(httpcn_addr.sin_port);
}

// 获取客户端IP地址
const char* HttpConn::getIp() const {
    // 将IP地址转换为字符串形式
    return inet_ntoa(httpcn_addr.sin_addr);
}

// 获取客户端地址结构体
sockaddr_in HttpConn::getAddr() const {
    return httpcn_addr;
}

// 处理HTTP请求
bool HttpConn::process() {
    // 初始化HTTP请求对象
    httpcn_request.initHttprq();

    // 检查读缓冲区是否有数据
    if (httpcn_read_buff.readableBytes() <= 0) {
        return false;
    } else if (httpcn_request.parse(httpcn_read_buff)) {
        // 请求解析成功，记录请求路径
        LOG_DEBUG("HttpConn.cpp: 112     %s", httpcn_request.path().c_str());
        // 初始化响应对象，状态码200
        httpcn_response.res_init(
            src_dir,
            httpcn_request.path(),
            httpcn_request.isKeepAlive(),
            200);
    } else {
        // 请求解析失败，返回400错误
        httpcn_response.res_init(src_dir, httpcn_request.path(), false, 400);
    }

    // 生成HTTP响应并写入缓冲区
    httpcn_response.makeResponse(httpcn_write_buff);

    // 设置第一个iovec指向响应头部数据
    httpcn_iovec[0].iov_base = const_cast<char*>(httpcn_write_buff.peek());
    httpcn_iovec[0].iov_len = httpcn_write_buff.readableBytes();
    httpcn_iocnt = 1;

    // 如果有文件数据，设置第二个iovec指向文件映射
    if (httpcn_response.fileLen() > 0 && httpcn_response.mmapFile()) {
        httpcn_iovec[1].iov_base = httpcn_response.mmapFile();
        httpcn_iovec[1].iov_len = httpcn_response.fileLen();
        httpcn_iocnt = 2;
    }

    // 记录文件大小和待写入数据量
    LOG_DEBUG(
        "HttpConn.cpp: 127     filesize:%d, %d to %d",
        httpcn_response.fileLen(),
        httpcn_iocnt,
        toWriteBytes());
    return true;
}

// 获取待写入的字节数
int HttpConn::toWriteBytes() {
    // 计算所有iovec中的数据总量
    return httpcn_iovec[0].iov_len + httpcn_iovec[1].iov_len;
}

// 判断是否为长连接
bool HttpConn::isKeepAlive() const {
    // 从请求对象获取连接类型
    return httpcn_request.isKeepAlive();
}
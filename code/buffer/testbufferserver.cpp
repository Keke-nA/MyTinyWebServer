#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/select.h>  // 确保定义 FD_SETSIZE
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>   // 定义 errno 相关常量
#include <cstring>  // 声明 strerror 函数
#include <iostream>
#include "buffer.hpp"  // 你的 Buffer 类头文件

#define MAX_EVENTS 10
#define PORT 8888
#define BACKLOG 10

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen_fd == -1) {
        std::cerr << "socket() failed" << std::endl;
        return 1;
    }

    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "bind() failed" << std::endl;
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        std::cerr << "listen() failed" << std::endl;
        close(listen_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        std::cerr << "epoll_create1() failed" << std::endl;
        close(listen_fd);
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;  // 边缘触发模式
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        std::cerr << "epoll_ctl() failed" << std::endl;
        close(listen_fd);
        close(epoll_fd);
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];
    Buffer client_buffers[FD_SETSIZE];  // 每个客户端的缓冲区

    std::cout << "Server started on port " << PORT << std::endl;

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            std::cerr << "epoll_wait() failed" << std::endl;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {  // 新连接
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int conn_fd = accept4(listen_fd, (struct sockaddr*)&client_addr, &addr_len, SOCK_NONBLOCK);
                if (conn_fd == -1) {
                    std::cerr << "accept4() failed" << std::endl;
                    continue;
                }

                // 初始化客户端缓冲区
                client_buffers[conn_fd] = Buffer(4096);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = conn_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &event) == -1) {
                    std::cerr << "epoll_ctl() for new connection failed" << std::endl;
                    close(conn_fd);
                }

                std::cout << "New connection: " << conn_fd << std::endl;
            } else {  // 数据就绪或错误
                if (events[i].events & EPOLLIN) {
                    // 读取数据到缓冲区
                    int saved_errno;
                    ssize_t bytes_read = client_buffers[fd].readFd(fd, &saved_errno);
                    if (bytes_read == -1) {
                        if (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK) {
                            // 正常情况，继续处理
                        } else {
                            std::cerr << "readFd() error: " << strerror(saved_errno) << std::endl;
                            close(fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        }
                    } else if (bytes_read == 0) {  // 客户端断开
                        std::cout << "Client disconnected" << std::endl;
                        close(fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    } else {
                        // 处理数据（例如回显）
                        std::string data = client_buffers[fd].retrieveAllToStr();
                        std::cout << "Received: " << data;
                        // 将数据写回客户端（回显）
                        client_buffers[fd].append(data);
                        event.events = EPOLLOUT | EPOLLET;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
                    }
                } else if (events[i].events & EPOLLOUT) {
                    // 写数据到客户端
                    int saved_errno;
                    ssize_t bytes_written = client_buffers[fd].writeFd(fd, &saved_errno);
                    if (bytes_written == -1) {
                        std::cerr << "writeFd() error: " << strerror(saved_errno) << std::endl;
                        close(fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    } else {
                        event.events = EPOLLIN | EPOLLET;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
                    }
                }
            }
        }
    }

    close(listen_fd);
    close(epoll_fd);
    return 0;
}
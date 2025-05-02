#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

// Epoller类：封装epoll相关操作，负责事件注册、修改、删除及等待
class Epoller {
  public:
    // 构造函数，初始化epoll实例和事件数组
    Epoller(int maxevent = 1024);
    // 析构函数，关闭epoll文件描述符
    ~Epoller();
    // 向epoll中注册新的文件描述符及其关注的事件
    bool addFd(int fd, size_t events);
    // 修改已注册文件描述符的事件类型
    bool modFd(int fd, size_t events);
    // 从epoll中移除文件描述符
    bool delDf(int fd);
    // 等待事件发生，返回就绪事件数量
    int wait(int timeoutms = -1);
    // 获取第i个就绪事件对应的文件描述符
    int getEventFd(size_t i) const;
    // 获取第i个就绪事件的事件类型
    uint32_t getEvents(size_t i) const;

  private:
    int epoll_fd;                           // epoll实例的文件描述符
    std::vector<struct epoll_event> events; // 存储就绪事件的数组
};
#include "Epoller.hpp"

// 构造函数：创建epoll实例并初始化事件数组
Epoller::Epoller(int maxevent) : epoll_fd(epoll_create1(0)), events(maxevent) {
    // 确保epoll实例创建成功且事件数组大小大于0
    assert(epoll_fd >= 0 && events.size() > 0);
}

// 析构函数：关闭epoll实例
Epoller::~Epoller() {
    // 释放epoll文件描述符
    close(epoll_fd);
}

// 向epoll实例添加文件描述符及其监听的事件
bool Epoller::addFd(int fd, size_t events) {
    // 检查文件描述符有效性
    if (fd < 0) {
        return false;
    }
    // 创建epoll事件结构体并设置
    struct epoll_event ev {};
    ev.data.fd = fd;
    ev.events = events;
    // 调用epoll_ctl添加文件描述符到epoll实例
    return 0 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

// 修改epoll实例中文件描述符的监听事件
bool Epoller::modFd(int fd, size_t events) {
    // 检查文件描述符有效性
    if (fd < 0) {
        return false;
    }
    // 创建epoll事件结构体并设置
    struct epoll_event ev {};
    ev.data.fd = fd;
    ev.events = events;
    // 调用epoll_ctl修改文件描述符的监听事件
    return 0 == epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

// 从epoll实例中删除文件描述符
bool Epoller::delDf(int fd) {
    // 检查文件描述符有效性
    if (fd < 0) {
        return false;
    }
    // 创建空的epoll事件结构体（删除操作不需要事件信息）
    struct epoll_event ev {};
    // 调用epoll_ctl从epoll实例中删除文件描述符
    return 0 == epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ev);
}

// 等待事件发生，返回就绪的事件数量
int Epoller::wait(int timeoutms) {
    // 调用epoll_wait等待事件发生，timeoutms为超时时间（毫秒）
    return epoll_wait(
        epoll_fd,
        &events[0],
        static_cast<int>(events.size()),
        timeoutms);
}

// 获取第i个事件对应的文件描述符
int Epoller::getEventFd(size_t i) const {
    // 确保索引在有效范围内
    assert(i < events.size() && i >= 0);
    // 返回事件对应的文件描述符
    return events[i].data.fd;
}

// 获取第i个事件的事件类型
uint32_t Epoller::getEvents(size_t i) const {
    // 确保索引在有效范围内
    assert(i < events.size() && i >= 0);
    // 返回事件类型
    return events[i].events;
}
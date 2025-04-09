#include "Epoller.hpp"

// 构造函数
Epoller::Epoller(int maxevent) : epoll_fd(epoll_create1(0)), events(maxevent) {
    // 待实现
    assert(epoll_fd >= 0 && events.size() > 0);
}

// 析构函数
Epoller::~Epoller() {
    // 待实现
    close(epoll_fd);
}

// 向 epoll 实例添加文件描述符及事件
bool Epoller::addFd(int fd, size_t events) {
    // 待实现
    if (fd < 0) {
        return false;
    }
    struct epoll_event ev{};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

// 修改 epoll 实例中文件描述符的事件
bool Epoller::modFd(int fd, size_t events) {
    // 待实现
    if (fd < 0) {
        return false;
    }
    struct epoll_event ev{};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

// 从 epoll 实例中删除文件描述符
bool Epoller::delDf(int fd) {
    // 待实现
    if (fd < 0) {
        return false;
    }
    struct epoll_event ev{};
    return 0 == epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ev);
}

// 等待事件发生
int Epoller::wait(int timeoutms) {
    // 待实现
    return epoll_wait(epoll_fd, &events[0], static_cast<int>(events.size()), timeoutms);
}

// 获取第 i 个事件对应的文件描述符
int Epoller::getEventFd(size_t i) const {
    // 待实现
    assert(i < events.size() && i > 0);
    return events[i].data.fd;
}

// 获取第 i 个事件的事件类型
uint32_t Epoller::getEvents(size_t i) const {
    // 待实现
    assert(i < events.size() && i > 0);
    return events[i].events;
}
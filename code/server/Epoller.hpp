#pragma once

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <vector>

class Epoller {
   public:
    Epoller(int maxevent = 1024);
    ~Epoller();
    bool addFd(int fd, size_t events);
    bool modFd(int fd, size_t events);
    bool delDf(int fd);
    int wait(int timeoutms = -1);
    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;

   private:
    int epoll_fd;
    std::vector<struct epoll_event> events;
};
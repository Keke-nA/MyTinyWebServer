#include <sys/time.h>
#include <time.h>
#include <iostream>

int main() {
    timeval now{0, 0};
    gettimeofday(&now, nullptr);
    time_t tsec = now.tv_sec;
    tm* systime = localtime(&tsec);
    tm t = *systime;
    std::cout << "当前时间:" << t.tm_year + 1900 << "-" << t.tm_mon + 1 << "-" << t.tm_mday << " " << t.tm_hour << ":"
              << t.tm_min << ":" << t.tm_sec << "." << now.tv_usec << std::endl;
    return 0;
}
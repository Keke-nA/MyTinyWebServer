#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "buffer.hpp"
#include <arpa/inet.h> // 添加此行

#define SERVER_IP "127.0.0.1"
#define PORT 8888

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd == -1) {
        std::cerr << "socket() failed" << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int connected = 0;
    while (!connected) {
        int ret = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (ret == 0) {
            connected = 1;
        } else if (errno != EINPROGRESS) {
            std::cerr << "connect() failed: " << strerror(errno) << std::endl;
            close(sockfd);
            return 1;
        }
    }

    std::cout << "Connected to server" << std::endl;

    Buffer client_buffer(4096);
    bool running = true;
    while (running) {
        // 发送数据到服务器
        std::string input;
        std::cout << "Enter message: ";
        std::getline(std::cin, input);
        if (input == "exit") {
            running = false;
        } else {
            client_buffer.append(input + "\n");
            int saved_errno;
            ssize_t bytes_sent = client_buffer.writeFd(sockfd, &saved_errno);
            if (bytes_sent == -1) {
                std::cerr << "writeFd() failed: " << strerror(saved_errno) << std::endl;
                break;
            }
        }

        // 读取服务器响应
        int saved_errno;
        ssize_t bytes_read = client_buffer.readFd(sockfd, &saved_errno);
        if (bytes_read > 0) {
            std::string response = client_buffer.retrieveAllToStr();
            std::cout << "Server response: " << response;
        } else if (bytes_read == -1 && saved_errno != EAGAIN) {
            std::cerr << "readFd() failed: " << strerror(saved_errno) << std::endl;
            running = false;
        }
    }

    close(sockfd);
    return 0;
}
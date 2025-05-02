#pragma once

#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>
//#include <atomic>
#include <string>
#include <vector>

// 缓冲区类，用于管理数据的读写操作
class Buffer {
   public:
    // 构造函数，初始化缓冲区大小，默认为 1024 字节
    Buffer(size_t initbuffersize = 1024);
    // 析构函数
    ~Buffer() = default;

    // 获取缓冲区可写的字节数
    const size_t writeableBytes() const;
    // 获取缓冲区可读的字节数
    const size_t readableBytes() const;
    // 获取缓冲区可前置写入的字节数
    const size_t prependableBytes() const;
    // 获取缓冲区可读数据的起始指针
    const char* peek() const;
    // 确保缓冲区有足够的空间来写入指定长度的数据
    void ensureWriteable(size_t len);
    // 标记已经写入了指定长度的数据
    void hasWritten(size_t len);
    // 标记已经读出了指定长度的数据
    void hasRead(size_t len);
    // 标记已经读取了指定长度的数据
    void retrieve(size_t len);
    // 读取数据直到指定的结束位置
    void retrieveUntill(const char* end);
    // 读取缓冲区中的所有数据
    void retrieveAll();
    // 将缓冲区中的所有数据读取为一个字符串
    std::string retrieveAllToStr();
    // 获取缓冲区可写位置的常量指针
    const char* beginWriteConst() const;
    // 获取缓冲区可写位置的指针
    char* beginWrite();
    // 向缓冲区追加一个字符串
    void append(const std::string& str);
    // 向缓冲区追加一个指定长度的字符数组
    void append(const char* str, size_t len);
    // 向缓冲区追加一个指定长度的数据
    void append(const void* data, size_t len);
    // 向缓冲区追加另一个 Buffer 对象的数据
    void append(const Buffer& buff);
    // 从文件描述符中读取数据到缓冲区，返回读取的字节数，errno 用于存储错误码
    ssize_t readFd(int fd, int* saveerrno);
    // 将缓冲区中的数据写入到文件描述符，返回写入的字节数，errno 用于存储错误码
    ssize_t writeFd(int fd, int* saveerrno);

   private:
    // 获取缓冲区的起始指针
    char* beginPtr();
    // 获取缓冲区的起始常量指针
    const char* beginPtr() const;
    // 确保缓冲区有足够的空间来容纳指定长度的数据
    void makeSpace(size_t len);

    // 存储数据的向量
    std::vector<char> buffer;
    // 读取位置的原子变量
    size_t read_pos;
    // 写入位置的原子变量
    size_t write_pos;
};
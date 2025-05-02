#include "Buffer.hpp"

// 构造函数，初始化缓冲区
Buffer::Buffer(size_t initbuffersize)
    : write_pos(0), read_pos(0), buffer(initbuffersize) {
}

// 获取缓冲区可写的字节数
const size_t Buffer::writeableBytes() const {
    return buffer.size() - write_pos;
}

// 获取缓冲区可读的字节数
const size_t Buffer::readableBytes() const {
    return write_pos - read_pos;
}

// 获取缓冲区可前置写入的字节数
const size_t Buffer::prependableBytes() const {
    return read_pos;
}

// 获取缓冲区可读数据的起始指针
const char* Buffer::peek() const {
    return beginPtr() + read_pos;
}

// 确保缓冲区有足够的空间来写入指定长度的数据
void Buffer::ensureWriteable(size_t len) {
    if (buffer.size() - write_pos < len) {
        // 空间不足，需要扩展
        makeSpace(len);
    }
}

// 标记已经写入了指定长度的数据
void Buffer::hasWritten(size_t len) {
    write_pos += len;
}

// 标记已经读出了指定长度的数据
void Buffer::hasRead(size_t len) {
    read_pos += len;
}

// 标记已经读取了指定长度的数据
void Buffer::retrieve(size_t len) {
    assert(len <= readableBytes());
    read_pos += len;
}

// 读取数据直到指定的结束位置
void Buffer::retrieveUntill(const char* end) {
    size_t temp = static_cast<size_t>(end - peek());
    assert(end <= beginWrite() && temp <= readableBytes());
    retrieve(temp);
}

// 读取缓冲区中的所有数据
void Buffer::retrieveAll() {
    read_pos = 0;
    write_pos = 0;
}

// 将缓冲区中的所有数据读取为一个字符串
std::string Buffer::retrieveAllToStr() {
    // 创建一个包含所有可读数据的字符串
    std::string temp(peek(), readableBytes());
    // 重置缓冲区
    retrieveAll();
    return std::move(temp);
}

// 获取缓冲区可写位置的常量指针
const char* Buffer::beginWriteConst() const {
    return buffer.data() + write_pos;
}

// 获取缓冲区可写位置的指针
char* Buffer::beginWrite() {
    return buffer.data() + write_pos;
}

// 向缓冲区追加一个字符串
void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

// 向缓冲区追加一个指定长度的字符数组
void Buffer::append(const char* str, size_t len) {
    // 确保有足够空间
    ensureWriteable(len);
    // 复制数据
    std::copy(str, str + len, beginWrite());
    // 更新写入位置
    hasWritten(len);
}

// 向缓冲区追加一个指定长度的数据
void Buffer::append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
}

// 向缓冲区追加另一个 Buffer 对象的数据
void Buffer::append(const Buffer& buff) {
    append(buff.peek(), buff.readableBytes());
}

// 从文件描述符中读取数据到缓冲区
ssize_t Buffer::readFd(int fd, int* saveerrno) {
    char buf[65535];
    // 准备两个缓冲区：一个是当前Buffer对象的可写区域，一个是栈上的临时缓冲区
    iovec iv[2];
    iv[0].iov_base = beginWrite();
    iv[0].iov_len = writeableBytes();
    iv[1].iov_base = buf;
    iv[1].iov_len = sizeof(buf);

    // 使用readv一次性读取数据到两个缓冲区
    const ssize_t rlen = readv(fd, iv, 2);

    if (rlen < 0) {
        // 读取出错
        *saveerrno = errno;
    } else if (static_cast<size_t>(rlen) <= writeableBytes()) {
        // 数据全部读入Buffer对象的可写区域
        hasWritten(rlen);
    } else {
        // 数据部分读入Buffer对象，部分读入栈上临时缓冲区
        int rawwriteable = writeableBytes();
        hasWritten(rawwriteable);
        // 将栈上临时缓冲区的数据追加到Buffer对象
        append(buf, rlen - rawwriteable);
    }
    return rlen;
}

// 将缓冲区中的数据写入到文件描述符
ssize_t Buffer::writeFd(int fd, int* saveerrno) {
    size_t readsize = readableBytes();
    // 将可读数据写入文件描述符
    const ssize_t wlen = write(fd, peek(), readsize);

    if (wlen < 0) {
        // 写入出错
        *saveerrno = errno;
    } else {
        // 更新已读位置
        hasRead(wlen);
    }
    return wlen;
}

// 获取缓冲区的起始指针
char* Buffer::beginPtr() {
    return buffer.data();
}

// 获取缓冲区的起始常量指针
const char* Buffer::beginPtr() const {
    return buffer.data();
}

// 确保缓冲区有足够的空间来容纳指定长度的数据
void Buffer::makeSpace(size_t len) {
    size_t left = prependableBytes();
    size_t right = writeableBytes();

    if (left + right >= len) {
        // 通过移动数据来腾出空间
        std::copy(
            buffer.begin() + read_pos,
            buffer.begin() + write_pos,
            buffer.begin());
        write_pos -= read_pos;
        read_pos = 0;
    } else {
        // 空间不足，需要扩展缓冲区
        buffer.resize(write_pos + len);
    }
}
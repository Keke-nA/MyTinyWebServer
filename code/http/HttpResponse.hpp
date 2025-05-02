// httpresponse.h
#pragma once

#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "../buffer/Buffer.hpp"

// HTTP响应处理类
class HttpResponse {
  public:
    // 构造函数
    HttpResponse();
    // 析构函数
    ~HttpResponse();

    // 初始化响应参数
    void res_init(
        const std::string& srcdir,
        std::string& path,
        bool iskeepalive = false,
        int code = -1);
    // 生成完整的HTTP响应
    void makeResponse(Buffer& buff);
    // 解除内存映射
    void unMapfile();
    // 获取映射文件指针
    char* mmapFile();
    // 获取文件长度
    size_t fileLen() const;
    // 生成错误响应内容
    void errorContent(Buffer& buff, std::string message);
    // 获取当前状态码
    int resCode() const;

  private:
    // 添加状态行到缓冲区
    void addStateLine(Buffer& buff);
    // 添加响应头到缓冲区
    void addHeader(Buffer& buff);
    // 添加响应体到缓冲区
    void addContent(Buffer& buff);
    // 设置错误页面路径
    void errorHtmlPath();
    // 获取文件MIME类型
    std::string getFileType();

    int http_code;                // HTTP状态码
    bool is_keepalive;            // 是否保持连接
    std::string http_path;        // 请求的文件路径
    std::string http_src_dir;     // 资源文件根目录
    char* http_mmfile;            // 内存映射文件指针
    struct stat http_mmfile_stat; // 文件状态信息

    // 文件后缀到MIME类型的映射
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    // 状态码到状态描述的映射
    static const std::unordered_map<int, std::string> CODE_STATUS;
    // 状态码到错误页面路径的映射
    static const std::unordered_map<int, std::string> CODE_PATH;
};
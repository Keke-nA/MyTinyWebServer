// httpresponse.h
#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <unordered_map>

#include "../buffer/Buffer.hpp"
#include "../log/Log.hpp"


/**
 * @brief HTTP响应处理类
 * 负责生成HTTP响应数据，包括状态行、响应头和响应体
 */
class HttpResponse {
public:
    /**
     * @brief 构造函数
     * 初始化HTTP响应对象
     */
    HttpResponse();

    /**
     * @brief 析构函数
     * 自动释放内存映射资源
     */
    ~HttpResponse();

    /**
     * @brief 初始化响应参数
     * @param srcdir 资源文件根目录
     * @param path 请求的文件路径
     * @param iskeepalive 是否保持连接（默认：false）
     * @param code HTTP状态码（默认：-1）
     */
    void res_init(const std::string& srcdir, std::string& path, bool iskeepalive = false, int code = -1);

    /**
     * @brief 生成完整的HTTP响应
     * @param buff 用于存储响应数据的缓冲区
     */
    void makeResponse(Buffer& buff);

    /**
     * @brief 解除内存映射
     * 释放mmap分配的内存资源
     */
    void unMapfile();

    /**
     * @brief 获取映射文件指针
     * @return char* 指向内存映射文件的指针
     */
    char* mmapFile();

    /**
     * @brief 获取文件长度
     * @return size_t 文件字节大小
     */
    size_t fileLen() const;

    /**
     * @brief 生成错误响应内容
     * @param buff 响应缓冲区
     * @param message 错误提示信息
     */
    void errorContent(Buffer& buff, std::string message);

    /**
     * @brief 获取当前状态码
     * @return int HTTP状态码
     */
    int resCode() const;

private:
    /**
     * @brief 添加状态行到缓冲区
     * @param buff 响应缓冲区
     */
    void addStateLine(Buffer& buff);

    /**
     * @brief 添加响应头到缓冲区
     * @param buff 响应缓冲区
     */
    void addHeader(Buffer& buff);

    /**
     * @brief 添加响应体到缓冲区
     * @param buff 响应缓冲区
     */
    void addContent(Buffer& buff);

    /**
     * @brief 设置错误页面路径
     * 根据状态码查找对应的错误页面
     */
    void errorHtmlPath();

    /**
     * @brief 获取文件MIME类型
     * @return std::string 文件对应的MIME类型
     */
    std::string getFileType();

    int http_code;                /**< HTTP状态码 */
    bool is_keepalive;       /**< 是否保持连接 */
    std::string http_path;   /**< 请求的文件路径 */
    std::string http_src_dir; /**< 资源文件根目录 */
    char* http_mmfile;       /**< 内存映射文件指针 */
    struct stat http_mmfile_stat; /**< 文件状态信息 */

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};
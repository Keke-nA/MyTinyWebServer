#pragma once

#include "../buffer/Buffer.hpp"
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

class HttpRequest {
  public:
    // HTTP请求解析的状态
    enum class PARSE_STATE { REQUEST_LINE, HEADERS, BODY, FINISH };
    // HTTP响应状态码
    enum class HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    HttpRequest();
    ~HttpRequest() = default;

    // 初始化HTTP请求
    void initHttprq();
    // 解析HTTP请求
    bool parse(Buffer& buff);
    // 获取请求路径(常量版本)
    const std::string& path() const;
    // 获取请求路径(非常量版本)
    std::string& path();
    // 获取请求方法
    std::string method() const;
    // 获取HTTP版本
    std::string version() const;
    // 获取POST请求参数(string版本)
    std::string getPost(const std::string& key) const;
    // 获取POST请求参数(char*版本)
    std::string getPost(const char* key) const;
    // 判断是否为长连接
    bool isKeepAlive() const;

  private:
    // 解析请求行
    bool parseRequestLine(const std::string& line);
    // 解析请求头
    void parseHeader(const std::string& line);
    // 解析请求体
    void parseBody(const std::string& line);
    // 解析请求路径
    void parsePath();
    // 解析POST请求
    void parsePost();
    // 解析URL编码的数据
    void parseFromUrlEncoded();
    // 用户验证
    static bool
    userVerify(const std::string& name, const std::string& pwd, bool islogin);
    // 转换十六进制字符
    static int converHex(char ch);

    PARSE_STATE httprq_state;   // 当前解析状态
    std::string httprq_method;  // 请求方法
    std::string httprq_path;    // 请求路径
    std::string httprq_version; // HTTP版本
    std::string httprq_body;    // 请求体
    std::unordered_map<std::string, std::string> httprq_header; // 请求头
    std::unordered_map<std::string, std::string> httprq_post; // POST请求参数

    static const std::unordered_set<std::string>
        DEFAULT_HTML; // 默认HTML页面集合
    static const std::unordered_map<std::string, int>
        DEFAULT_HTML_TAG; // HTML页面标签映射
};
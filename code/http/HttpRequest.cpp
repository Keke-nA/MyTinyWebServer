#include "HttpRequest.hpp"
#include "../log/Log.hpp"
#include "../pool/SqlConnRAII.hpp"
#include <algorithm>
#include <regex>

// 初始化静态成员变量
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login", "/welcome", "/video", "/picture"};
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0}, {"/login.html", 1}};

// 构造函数
HttpRequest::HttpRequest() {
    initHttprq();
}

// 初始化函数
void HttpRequest::initHttprq() {
    // 初始化解析状态为请求行
    httprq_state = PARSE_STATE::REQUEST_LINE;
    // 清空所有字符串成员
    httprq_method = httprq_path = httprq_version = httprq_body = "";
    // 清空请求头和POST数据容器
    httprq_header.clear();
    httprq_post.clear();
}

// 解析HTTP请求
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    // 检查缓冲区是否有可读数据
    if (buff.readableBytes() <= 0) {
        return false;
    }

    // 循环解析请求，直到缓冲区为空或解析完成
    while (buff.readableBytes() && httprq_state != PARSE_STATE::FINISH) {
        // 查找行结束符（CRLF）
        const char* lineend =
            std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.peek(), lineend);

        // 根据当前解析状态处理数据
        switch (httprq_state) {
        case PARSE_STATE::REQUEST_LINE:
            // 解析请求行，失败则返回false
            if (!parseRequestLine(line)) {
                return false;
            }
            // 解析请求路径
            parsePath();
            break;
        case PARSE_STATE::HEADERS:
            // 解析请求头
            parseHeader(line);
            // 如果缓冲区剩余数据不足，则认为请求解析完成
            if (buff.readableBytes() <= 2) {
                httprq_state = PARSE_STATE::FINISH;
            }
            break;
        case PARSE_STATE::BODY:
            // 解析请求体
            parseBody(line);
            break;
        default:
            break;
        }

        // 如果已经到达缓冲区末尾，则退出循环
        if (lineend == buff.beginWrite()) {
            break;
        }
        // 移动缓冲区读指针，跳过已处理的数据
        buff.retrieveUntill(lineend + 2);
    }

    // 记录请求行信息到日志
    LOG_DEBUG(
        "HttpRequest.cpp: 56     请求行：[%s] [%s] [%s]",
        httprq_method.c_str(),
        httprq_path.c_str(),
        httprq_version.c_str());
    return true;
}

// 获取路径的常量版本
const std::string& HttpRequest::path() const {
    return httprq_path;
}

// 获取路径的非常量版本
std::string& HttpRequest::path() {
    return httprq_path;
}

// 获取请求方法
std::string HttpRequest::method() const {
    return httprq_method;
}

// 获取HTTP版本
std::string HttpRequest::version() const {
    return httprq_version;
}

// 根据string类型的键获取POST数据
std::string HttpRequest::getPost(const std::string& key) const {
    // 确保键不为空
    assert(key != "");
    // 查找并返回对应的值，如果不存在则返回空字符串
    if (httprq_post.count(key) == 1) {
        return httprq_post.find(key)->second;
    }
    return "";
}

// 根据const char*类型的键获取POST数据
std::string HttpRequest::getPost(const char* key) const {
    // 确保键不为空
    assert(key != nullptr);
    // 转换为string类型并调用对应的重载函数
    return getPost(std::string(key));
}

// 判断是否为长连接
bool HttpRequest::isKeepAlive() const {
    // 检查Connection头是否存在
    if (httprq_header.count("Connection") == 1) {
        // HTTP/1.1且Connection为keep-alive时返回true
        return httprq_header.find("Connection")->second == "keep-alive"
               && httprq_version == "1.1";
    }
    return false;
}

// 解析请求行
bool HttpRequest::parseRequestLine(const std::string& line) {
    // 使用正则表达式匹配请求行格式：METHOD PATH HTTP/VERSION
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, patten)) {
        // 提取请求方法、路径和HTTP版本
        httprq_method = submatch[1];
        httprq_path = submatch[2];
        httprq_version = submatch[3];
        // 更新解析状态为请求头
        httprq_state = PARSE_STATE::HEADERS;
        return true;
    }
    // 请求行格式错误，记录日志并返回false
    LOG_ERROR("HttpRequest.cpp: 122     RequestLine Error");
    return false;
}

// 解析请求头
void HttpRequest::parseHeader(const std::string& line) {
    // 使用正则表达式匹配请求头格式：Key: Value
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, patten)) {
        // 将键值对存入请求头映射
        httprq_header[submatch[1]] = submatch[2];
    } else {
        // 遇到空行，表示请求头结束，开始解析请求体
        httprq_state = PARSE_STATE::BODY;
    }
}

// 解析请求体
void HttpRequest::parseBody(const std::string& line) {
    // 保存请求体内容
    httprq_body = line;
    // 解析POST数据
    parsePost();
    // 更新解析状态为完成
    httprq_state = PARSE_STATE::FINISH;
    // 记录请求体信息到日志
    LOG_DEBUG(
        "HttpRequest.cpp: 143     Body:%s, len:%d",
        line.c_str(),
        line.size());
}

// 解析路径
void HttpRequest::parsePath() {
    // 处理根路径，默认为index.html
    if (httprq_path == "/") {
        httprq_path = "/index.html";
    } else {
        // 检查是否为预定义路径，如果是则添加.html后缀
        for (const auto& item : DEFAULT_HTML) {
            if (item == httprq_path) {
                httprq_path += ".html";
                break;
            }
        }
    }
}

// 解析POST请求
void HttpRequest::parsePost() {
    // 检查是否为POST请求且内容类型为表单数据
    if (httprq_method == "POST"
        && httprq_header["Content-Type"]
               == "application/x-www-form-urlencoded") {
        // 解析URL编码的表单数据
        parseFromUrlEncoded();
        // 处理登录和注册请求
        if (DEFAULT_HTML_TAG.count(httprq_path) != 0) {
            int tag = DEFAULT_HTML_TAG.find(httprq_path)->second;
            LOG_DEBUG("HttpRequest.cpp: 168     Tag:%d", tag);
            // 0表示注册，1表示登录
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                // 验证用户信息
                if (userVerify(
                        httprq_post["username"],
                        httprq_post["password"],
                        isLogin)) {
                    // 验证成功，重定向到欢迎页面
                    httprq_path = "/welcome.html";
                } else {
                    // 验证失败，重定向到错误页面
                    httprq_path = "/error.html";
                }
            }
        }
    }
}

// 解析URL编码的数据
void HttpRequest::parseFromUrlEncoded() {
    // 请求体为空则直接返回
    if (httprq_body.size() == 0) {
        return;
    }

    std::string key, value;
    int num = 0;
    int n = httprq_body.size();
    int i = 0, j = 0;

    // 逐字符解析URL编码的表单数据
    for (; i < n; i++) {
        char ch = httprq_body[i];
        switch (ch) {
        case '=':
            // 遇到等号，提取键名
            key = httprq_body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            // 将加号转换为空格
            httprq_body[i] = ' ';
            break;
        case '%':
            // 处理百分号编码（如%20表示空格）
            num = converHex(httprq_body[i + 1]) * 16
                  + converHex(httprq_body[i + 2]);
            httprq_body[i] = static_cast<char>(num);
            // 删除后续两个字符（十六进制编码）
            httprq_body.erase(i + 1, 2);
            break;
        case '&':
            // 遇到&符号，提取键值并存入映射
            value = httprq_body.substr(j, i - j);
            j = i + 1;
            httprq_post[key] = value;
            // 记录键值对到日志
            LOG_DEBUG(
                "HttpRequest.cpp: 210     %s = %s",
                key.c_str(),
                value.c_str());
            break;
        default:
            break;
        }
    }

    // 处理最后一个键值对（没有&结尾的情况）
    assert(j <= i);
    if (httprq_post.count(key) == 0 && j < i) {
        value = httprq_body.substr(j, i - j);
        httprq_post[key] = value;
    }
}

// 用户验证
bool HttpRequest::userVerify(
    const std::string& name, const std::string& pwd, bool isLogin) {
    // 检查用户名和密码是否为空
    if (name == "" || pwd == "") {
        return false;
    }

    // 记录验证信息到日志
    LOG_INFO(
        "HttpRequest.cpp: 233     Verify name:%s pwd:%s",
        name.c_str(),
        pwd.c_str());

    // 获取数据库连接
    MYSQL* sql;
    SqlConnRAII sqlConn(&sql, &SqlConnPool::instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;

    // 注册模式下初始设置为成功
    if (!isLogin) {
        flag = true;
    }

    // 构造SQL查询语句，检查用户是否存在
    snprintf(
        order,
        256,
        "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
        name.c_str());
    LOG_DEBUG("HttpRequest.cpp: 249     sql query: %s", order);

    // 执行SQL查询
    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }

    // 获取查询结果
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    // 处理查询结果
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("HttpRequest.cpp: 260     MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);

        if (isLogin) {
            // 登录模式：验证密码是否匹配
            if (pwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_DEBUG("HttpRequest.cpp: 267     pwd error!");
            }
        } else {
            // 注册模式：用户名已存在，注册失败
            flag = false;
            LOG_DEBUG("HttpRequest.cpp: 271     user used!");
        }
    }
    mysql_free_result(res);

    // 处理注册请求
    if (!isLogin && flag == true) {
        LOG_DEBUG("HttpRequest.cpp: 276     register!");
        bzero(order, 256);
        // 构造插入用户的SQL语句
        snprintf(
            order,
            256,
            "INSERT INTO user(username, password) VALUES('%s','%s')",
            name.c_str(),
            pwd.c_str());
        LOG_DEBUG("HttpRequest.cpp: 279     sql insert: %s", order);

        // 执行插入操作
        if (mysql_query(sql, order)) {
            LOG_DEBUG("HttpRequest.cpp: 281     Insert error!");
            flag = false;
        }
        flag = true;
    }

    // 释放数据库连接
    SqlConnPool::instance().freeConn(sql);
    LOG_DEBUG("HttpRequest.cpp: 286     UserVerify success!!");
    return flag;
}

// 转换十六进制字符
int HttpRequest::converHex(char ch) {
    // 处理大写十六进制字符（A-F）
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    // 处理小写十六进制字符（a-f）
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    // 处理数字字符（0-9）
    return ch - '0';
}
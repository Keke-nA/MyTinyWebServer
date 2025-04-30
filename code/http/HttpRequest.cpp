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
    // 调用初始化函数
    initHttprq();
}

// 初始化函数
void HttpRequest::initHttprq() {
    // 初始化成员变量，如清空字符串、重置状态等
    httprq_state = PARSE_STATE::REQUEST_LINE;
    httprq_method = httprq_path = httprq_version = httprq_body = "";
    httprq_header.clear();
    httprq_post.clear();
}

// 解析函数
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if (buff.readableBytes() <= 0) {
        return false;
    }
    while (buff.readableBytes() && httprq_state != PARSE_STATE::FINISH) {
        const char* lineend =
            std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.peek(), lineend);
        // std::cout << line << std::endl;
        switch (httprq_state) {
        case PARSE_STATE::REQUEST_LINE:
            if (!parseRequestLine(line)) {
                return false;
            }
            parsePath();
            break;
        case PARSE_STATE::HEADERS:
            parseHeader(line);
            if (buff.readableBytes() <= 2) {
                httprq_state = PARSE_STATE::FINISH;
            }
            break;
        case PARSE_STATE::BODY:
            parseBody(line);
            break;
        default:
            break;
        }
        if (lineend == buff.beginWrite()) {
            break;
        }
        buff.retrieveUntill(lineend + 2);
    }
    LOG_DEBUG(
        "HttpRequest.cpp: 56     请求行：[%s] [%s] [%s]",
        httprq_method.c_str(),
        httprq_path.c_str(),
        httprq_version.c_str());
    return true;
}

// 获取路径的常量版本
const std::string& HttpRequest::path() const {
    // 返回路径
    return httprq_path;
}

// 获取路径的非常量版本
std::string& HttpRequest::path() {
    // 返回路径
    return httprq_path;
}

// 获取请求方法
std::string HttpRequest::method() const {
    // 返回请求方法
    return httprq_method;
}

// 获取 HTTP 版本
std::string HttpRequest::version() const {
    // 返回 HTTP 版本
    return httprq_version;
}

// 根据 std::string 类型的键获取 POST 数据
std::string HttpRequest::getPost(const std::string& key) const {
    // 查找并返回对应的值
    assert(key != "");
    if (httprq_post.count(key) == 1) {
        return httprq_post.find(key)->second;
    }
    return "";
}

// 根据 const char* 类型的键获取 POST 数据
std::string HttpRequest::getPost(const char* key) const {
    assert(key != nullptr);
    return getPost(std::string(key));
}

// 判断是否为长连接
bool HttpRequest::isKeepAlive() const {
    // 根据请求头判断是否为长连接
    if (httprq_header.count("Connection") == 1) {
        return httprq_header.find("Connection")->second == "keep-alive"
               && httprq_version == "1.1";
    }
    return false;
}

// 解析请求行
bool HttpRequest::parseRequestLine(const std::string& line) {
    // 解析请求行，提取方法、路径和版本
    // std::cout << "HttpRequest.cpp : 113  " << line << std::endl;
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, patten)) {
        httprq_method = submatch[1];
        httprq_path = submatch[2];
        httprq_version = submatch[3];
        httprq_state = PARSE_STATE::HEADERS;
        // std::cout << "HttpRequest.cpp : 122  " << httprq_method << "  " <<
        // httprq_path << "  " << httprq_version << "  " ; std::cout <<
        // httprq_state << std::endl;
        return true;
    }
    LOG_ERROR("HttpRequest.cpp: 122     RequestLine Error");
    return false;
}

// 解析请求头
void HttpRequest::parseHeader(const std::string& line) {
    // 解析请求头，将键值对存入 httprq_header
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, patten)) {
        httprq_header[submatch[1]] = submatch[2];
    } else {
        httprq_state = PARSE_STATE::BODY;
    }
}

// 解析请求体
void HttpRequest::parseBody(const std::string& line) {
    // 解析请求体，调用 parsePost 处理 POST 数据
    httprq_body = line;
    parsePost();
    httprq_state = PARSE_STATE::FINISH;
    LOG_DEBUG(
        "HttpRequest.cpp: 143     Body:%s, len:%d",
        line.c_str(),
        line.size());
}

// 解析路径
void HttpRequest::parsePath() {
    // 根据默认 HTML 列表处理路径
    if (httprq_path == "/") {
        httprq_path = "/index.html";
    } else {
        for (const auto& item : DEFAULT_HTML) {
            if (item == httprq_path) {
                httprq_path += ".html";
                break;
            }
        }
    }
}

// 解析 POST 数据
void HttpRequest::parsePost() {
    // 处理 POST 数据，调用 parseFromUrlEncoded 解析 URL 编码数据
    if (httprq_method == "POST"
        && httprq_header["Content-Type"]
               == "application/x-www-form-urlencoded") {
        parseFromUrlEncoded();
        if (DEFAULT_HTML_TAG.count(httprq_path) != 0) {
            int tag = DEFAULT_HTML_TAG.find(httprq_path)->second;
            LOG_DEBUG("HttpRequest.cpp: 168     Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (userVerify(
                        httprq_post["username"],
                        httprq_post["password"],
                        isLogin)) {
                    httprq_path = "/welcome.html";
                } else {
                    httprq_path = "/error.html";
                }
            }
        }
    }
}

// 解析 URL 编码的数据
void HttpRequest::parseFromUrlEncoded() {
    // 解析 URL 编码的数据，将键值对存入 httprq_post
    if (httprq_body.size() == 0) {
        return;
    }
    std::string key, value;
    int num = 0;
    int n = httprq_body.size();
    int i = 0, j = 0;
    for (; i < n; i++) {
        char ch = httprq_body[i];
        switch (ch) {
        case '=':
            key = httprq_body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            httprq_body[i] = ' ';
            break;
        case '%':
            num = converHex(httprq_body[i + 1]) * 16
                  + converHex(httprq_body[i + 2]);
            httprq_body[i] = static_cast<char>(num);
            httprq_body.erase(i + 1, 2);
            break;
        case '&':
            value = httprq_body.substr(j, i - j);
            j = i + 1;
            httprq_post[key] = value;
            LOG_DEBUG(
                "HttpRequest.cpp: 210     %s = %s",
                key.c_str(),
                value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (httprq_post.count(key) == 0 && j < i) {
        value = httprq_body.substr(j, i - j);
        httprq_post[key] = value;
    }
}

// 用户验证
bool HttpRequest::userVerify(
    const std::string& name, const std::string& pwd, bool islogin) {
    // 进行用户验证，与数据库交互等
    if (name == "" || pwd == "") {
        return false;
    }
    LOG_INFO(
        "HttpRequest.cpp: 229     Verify name:%s pwd:%s",
        name.c_str(),
        pwd.c_str());
    MYSQL* sql;
    SqlConnRAII sql_raii(&sql, &SqlConnPool::instance());
    assert(sql);
    bool flag = false;
    size_t j = 0;
    char order[256] = {0};
    MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;
    if (!islogin) {
        flag = true;
    }
    snprintf(
        order,
        256,
        "SELECT username, password FROM user WHERE username ='%s' LIMIT 1",
        name.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order)) {
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("HttpRequest.cpp: 250     MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        if (islogin) {
            if (pwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_DEBUG("HttpRequest.cpp: 257     pwd error!");
            }
        } else {
            flag = false;
            LOG_DEBUG("HttpRequest.cpp: 261     user used!");
        }
    }
    mysql_free_result(res);
    if (!islogin && flag == true) {
        LOG_DEBUG("register!");
        memset(order, 0, sizeof(order));
        snprintf(
            order,
            256,
            "INSERT INTO user(username, password) VALUES('%s','%s')",
            name.c_str(),
            pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("HttpRequest.cpp: 271     Insert error!");
            flag = false;
        } else {
            flag = true;
        }
    }
    SqlConnPool::instance().freeConn(sql);
    LOG_DEBUG("HttpRequest.cpp: 278     UserVerify success!");
    return flag;
}

// 转换十六进制字符
int HttpRequest::converHex(char ch) {
    // 将十六进制字符转换为对应的整数值
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 10;
    }
    return ch;
}
#pragma once

#include <algorithm>
#include <cstring>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../buffer/buffer.hpp"
#include "../log/Log.hpp"
#include "../pool/SqlConnRAII.hpp"

class HttpRequest {
   public:
    enum class PARSE_STATE { REQUEST_LINE, HEADERS, BODY, FINISH };
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
    void initHttprq();
    bool parse(Buffer& buff);
    const std::string& path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;
    bool isKeepAlive() const;

   private:
    bool parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    void parseBody(const std::string& line);
    void parsePath();
    void parsePost();
    void parseFromUrlEncoded();
    static bool userVerify(const std::string& name, const std::string& pwd, bool islogin);
    static int converHex(char ch);
    PARSE_STATE httprq_state;
    std::string httprq_method;
    std::string httprq_path;
    std::string httprq_version;
    std::string httprq_body;
    std::unordered_map<std::string, std::string> httprq_header;
    std::unordered_map<std::string, std::string> httprq_post;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};
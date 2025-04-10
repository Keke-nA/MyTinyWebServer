// httpresponse.cpp
#include "HttpResponse.hpp"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},          {".xml", "text/xml"},          {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},          {".rtf", "application/rtf"},   {".pdf", "application/pdf"},
    {".word", "application/nsword"}, {".png", "image/png"},         {".gif", "image/gif"},
    {".jpg", "image/jpeg"},          {".jpeg", "image/jpeg"},       {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},         {".mpg", "video/mpeg"},        {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},   {".tar", "application/x-tar"}, {".css", "text/css "},
    {".js", "text/javascript "},     {".ico", "image/x - icon"}  // 添加.ico对应的MIME类型
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

// 构造函数实现
HttpResponse::HttpResponse()
    : http_code(0), is_keepalive(false), http_path(""), http_src_dir(""), http_mmfile(nullptr), http_mmfile_stat({0}) {}

// 析构函数实现
HttpResponse::~HttpResponse() {
    // 释放内存映射资源
    unMapfile();
}

// 初始化响应参数
void HttpResponse::res_init(const std::string& srcdir, std::string& path, bool iskeepalive, int code) {
    // 参数校验
    assert(!srcdir.empty());
    // 释放之前的内存映射
    if (http_mmfile) {
        unMapfile();
    }
    // 初始化成员变量
    http_code = code;
    http_path = path;
    http_src_dir = srcdir;
    is_keepalive = iskeepalive;
    http_mmfile = nullptr;
    http_mmfile_stat = {0};
}

// 生成完整HTTP响应
void HttpResponse::makeResponse(Buffer& buff) {
    // 1. 检查文件状态
    if (stat((http_src_dir + http_path).data(), &http_mmfile_stat) < 0 || S_ISDIR(http_mmfile_stat.st_mode)) {
        http_code = 404;
    } else if (!(http_mmfile_stat.st_mode & S_IROTH)) {
        http_code = 403;
    } else if (http_code == -1) {
        http_code = 200;
    }
    // 2. 设置状态码
    // 3. 处理错误页面
    errorHtmlPath();
    // 4. 生成响应行
    addStateLine(buff);
    // 5. 生成响应头
    addHeader(buff);
    // 6. 生成响应体
    addContent(buff);
}

// 解除内存映射
void HttpResponse::unMapfile() {
    if (http_mmfile) {
        munmap(http_mmfile, http_mmfile_stat.st_size);
        http_mmfile = nullptr;
    }
}

// 获取映射文件指针
char* HttpResponse::mmapFile() {
    return http_mmfile;
}

// 获取文件长度
size_t HttpResponse::fileLen() const {
    return http_mmfile_stat.st_size;
}

// 生成错误响应内容
void HttpResponse::errorContent(Buffer& buff, std::string message) {
    // 构造错误响应体
    // 添加Content-Length头
    // 添加响应体内容
    std::string body{""};
    std::string status{""};
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(http_code) == 1) {
        status = CODE_STATUS.find(http_code)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(http_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";
    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}

// 获取当前状态码
int HttpResponse::resCode() const {
    return http_code;
}

// 添加状态行
void HttpResponse::addStateLine(Buffer& buff) {
    // 构造HTTP/1.1 200 OK格式
    std::string status{""};
    if (CODE_STATUS.count(http_code) != 1) {
        http_code = 400;
    }
    status = CODE_STATUS.find(http_code)->second;
    buff.append("HTTP/1.1 " + std::to_string(http_code) + " " + status + "\r\n");
}

// 添加响应头
void HttpResponse::addHeader(Buffer& buff) {
    // 添加Connection头
    // 添加Content-Type头
    // 添加Content-Length头
    buff.append("Connection: ");
    if (is_keepalive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.append("clode\r\n");
    }
    buff.append("Content-type: " + getFileType() + "\r\n");
}

// 添加响应体
void HttpResponse::addContent(Buffer& buff) {
    // 打开文件
    // 内存映射
    // 添加文件内容到缓冲区
    int srcfd = open((http_src_dir + http_path).data(), O_RDONLY);
    if (srcfd == -1) {
        errorContent(buff, "File NotFount!");
        return;
    }
    LOG_DEBUG("HttpResponse.cpp: 151     file path %s", (http_src_dir + http_path).data());
    int* mmret = (int*)mmap(0, http_mmfile_stat.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    if (*mmret == -1) {
        errorContent(buff, "File NotFound!");
        return;
    }
    http_mmfile = (char*)mmret;
    close(srcfd);
    buff.append("Content-length: " + std::to_string(http_mmfile_stat.st_size) + "\r\n\r\n");
    buff.append(http_mmfile, http_mmfile_stat.st_size);
}

// 设置错误页面路径
void HttpResponse::errorHtmlPath() {
    // 根据状态码查找对应的错误页面路径
    // 更新http_path和文件状态
    if (CODE_PATH.count(http_code) != 0) {
        http_path = CODE_PATH.find(http_code)->second;
        stat((http_src_dir + http_path).data(), &http_mmfile_stat);
    }
}

// 获取文件MIME类型
std::string HttpResponse::getFileType() {
    // 解析文件扩展名
    // 查找预定义的MIME类型映射
    // 返回对应类型或默认类型
    std::string::size_type idx = http_path.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = http_path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}
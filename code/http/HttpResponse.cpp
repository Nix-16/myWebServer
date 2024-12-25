#include "HttpResponse.h"

// 静态成员初始化
const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"txt", "text/plain"},
    {"xml", "application/xml"},
    {"pdf", "application/pdf"}};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {404, "Not Found"},
    {500, "Internal Server Error"}};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {404, "/404.html"},
    {500, "/500.html"}};

HttpResponse::HttpResponse()
    : code_(-1), isKeepAlive_(false), mmFile_(nullptr)
{
}

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::Init(const std::string &srcDir, const std::string &path, bool isKeepAlive, int code)
{
    this->srcDir_ = srcDir;
    this->path_ = path;
    this->isKeepAlive_ = isKeepAlive;
    this->code_ = code;

    // 构建文件的绝对路径
    std::string fullPath = srcDir + path;
    int ret = stat(fullPath.c_str(), &mmFileStat_);
    if (ret == -1)
    {
        // 文件不存在
        code_ = 404;
        return;
    }

    // 使用 mmap 映射文件
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        code_ = 500;
        return;
    }
    mmFile_ = (char *)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (mmFile_ == MAP_FAILED)
    {
        mmFile_ = nullptr;
        code_ = 500;
    }
}

void HttpResponse::MakeResponse(Buffer &buff)
{
    if (code_ == -1)
    {
        // 如果没有设置状态码，默认为 200
        code_ = 200;
    }

    // 构建响应
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

void HttpResponse::UnmapFile()
{
    if (mmFile_ != nullptr)
    {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

char *HttpResponse::File()
{
    return mmFile_;
}

size_t HttpResponse::FileLen() const
{
    return mmFileStat_.st_size;
}

void HttpResponse::ErrorContent(Buffer &buff, const std::string &message)
{
    std::string body = "<html><body><h1>" + message + "</h1></body></html>";
    buff.append(body);
}

std::string HttpResponse::GetFileType_()
{
    size_t pos = path_.find_last_of('.');
    if (pos == std::string::npos)
        return "text/plain"; // 默认类型为 text/plain

    std::string extension = path_.substr(pos + 1);
    auto it = SUFFIX_TYPE.find(extension);
    return it != SUFFIX_TYPE.end() ? it->second : "text/plain"; // 查找文件类型
}

void HttpResponse::AddStateLine_(Buffer &buff)
{
    buff.append("HTTP/1.1 ");
    buff.append(std::to_string(code_));
    buff.append(" ");
    buff.append(CODE_STATUS.at(code_));
    buff.append("\r\n");
}

void HttpResponse::AddHeader_(Buffer &buff)
{
    buff.append("Connection: ");
    buff.append(isKeepAlive_ ? "keep-alive" : "close");
    buff.append("\r\n");

    buff.append("Content-Type: ");
    buff.append(GetFileType_());
    buff.append("\r\n");

    if (code_ == 200)
    {
        buff.append("Content-Length: ");
        buff.append(std::to_string(FileLen()));
        buff.append("\r\n");
    }
    buff.append("\r\n");
}

void HttpResponse::AddContent_(Buffer &buff)
{
    if (code_ == 404 || code_ == 500)
    {
        ErrorContent(buff, "Something went wrong!");
    }
    else
    {
        buff.append(mmFile_, FileLen());
    }
}

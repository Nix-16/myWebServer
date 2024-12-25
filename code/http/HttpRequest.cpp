#include "HttpRequest.h"
#include <iostream>
#include <algorithm>

using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

// 初始化请求对象
HttpRequest::HttpRequest()
{
    Init();
}

// 重置请求对象状态
void HttpRequest::Init()
{
    state_ = REQUEST_LINE;
    method_.clear();
    path_.clear();
    version_.clear();
    body_.clear();
    header_.clear();
    post_.clear();
}

bool HttpRequest::parse(Buffer &buff)
{
    const char CRLF[] = "\r\n"; // 定义回车换行符

    if (buff.readableBytes() <= 0)
    { // 如果缓冲区没有数据，返回false
        return false;
    }

    while (buff.readableBytes() && state_ != FINISH)
    {
        // 检查当前状态
        if (state_ == BODY)
        {
            // 直接处理整个请求体
            std::string body(buff.peek(), buff.peek() + buff.readableBytes());
            ParseBody_(body);                    // 解析请求体
            buff.retrieve(buff.readableBytes()); // 清空已读取的 body 数据
            state_ = FINISH;                     // BODY 解析完成，状态置为 FINISH
            break;
        }

        // 其他状态（REQUEST_LINE 和 HEADERS）
        const char *lineEnd = std::search(buff.peek(), buff.peek() + buff.readableBytes(), CRLF, CRLF + 2);
        if (lineEnd == buff.peek() + buff.readableBytes())
        {
            // 没有找到 "\r\n"，等待更多数据
            break;
        }

        std::string line(buff.peek(), lineEnd); // 提取当前行

        switch (state_)
        {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line))
            {
                return false; // 解析请求行失败，返回false
            }
            ParsePath_(); // 解析路径（如果需要）
            break;

        case HEADERS:
            ParseHeader_(line); // 解析请求头

            // 检查是否遇到空行，表示请求头结束
            if (line.empty())
            {
                if (method_ == "POST")
                {
                    state_ = BODY; // 如果是 POST 请求，转到 BODY 状态
                }
                else
                {
                    state_ = FINISH; // 否则，解析完成
                }
            }
            break;

        default:
            break;
        }

        // 安全检查 lineEnd 是否有效
        if (lineEnd >= buff.peek() && lineEnd <= buff.peek() + buff.readableBytes())
        {
            size_t lineLength = lineEnd - buff.peek() + 2; // 行长度，包括 \r\n
            buff.retrieve(lineLength);                     // 从缓冲区中删除已读取的数据
        }
        else
        {
            std::cerr << "Invalid lineEnd pointer detected." << std::endl;
            return false; // 如果 lineEnd 无效，返回 false 避免段错误
        }
    }

    return true; // 返回true，表示成功解析
}

// 解析请求行
bool HttpRequest::ParseRequestLine_(const std::string &line)
{
    std::regex pattern(R"((GET|POST) (\S+) (HTTP/\d\.\d))");
    std::smatch match;
    if (std::regex_match(line, match, pattern))
    {
        method_ = match[1];
        path_ = match[2];
        version_ = match[3];
        return true;
    }
    return false;
}

// 解析请求头
void HttpRequest::ParseHeader_(const std::string &line)
{
    size_t pos = line.find(':');
    if (pos != std::string::npos)
    {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 2); // 忽略": "部分
        header_[key] = value;
    }
}

// 解析请求体
void HttpRequest::ParseBody_(const std::string &line)
{
    body_ = line; // 简单地将请求体附加到 body 字符串上
    ParsePost_();
    this->state_ = PARSE_STATE::FINISH;
}

// 获取POST请求表单中的参数
std::string HttpRequest::GetPost(const std::string &key) const
{
    if (post_.find(key) != post_.end())
    {
        return post_.at(key);
    }
    return "";
}

// 判断是否为长连接
// bool HttpRequest::IsKeepAlive() const
// {
//     return (header_.find("Connection") != header_.end()) &&
//            (header_.at("Connection") == "keep-alive");
// }
bool HttpRequest::IsKeepAlive() const
{
    auto it = header_.find("Connection");
    if (it != header_.end())
    {
        return it->second == "keep-alive";
    }
    return false; // 默认为短连接
}

// 请求路径处理
void HttpRequest::ParsePath_()
{
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
    this->state_ = PARSE_STATE::HEADERS;
}

// POST请求的表单参数处理
void HttpRequest::ParsePost_()
{
    ParseFormData_(); // 解析 POST 请求体中的表单数据
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlencoded_();

        if (DEFAULT_HTML_TAG.count(path_))
        {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if (tag == 0 || tag == 1)
            {
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin))
                {
                    path_ = "/welcome.html";
                }
                else
                {
                    path_ = "/error.html";
                }
            }
        }
    }
}

// 解析URL编码的数据
void HttpRequest::ParseFromUrlencoded_()
{
    if (body_.size() == 0)
    {
        return;
    }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
    if (post_.count(key) == 0 && j < i)
    {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// 解析POST表单数据
void HttpRequest::ParseFormData_()
{
    std::istringstream bodyStream(body_);
    std::string keyValue;
    while (std::getline(bodyStream, keyValue, '&'))
    {
        size_t pos = keyValue.find('=');
        if (pos != std::string::npos)
        {
            std::string key = keyValue.substr(0, pos);
            std::string value = keyValue.substr(pos + 1);
            post_[key] = value;
        }
    }
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin)
{
    // 检查用户名或密码是否为空
    if (name.empty() || pwd.empty())
    {
        std::cerr << "Username or password is empty." << std::endl;
        return false;
    }

    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance()); // 获取数据库连接

    bool flag = false;
    char order[256] = {0};
    MYSQL_RES *res = nullptr;

    // 登录逻辑
    if (isLogin)
    {
        snprintf(order, 256, "SELECT password FROM user WHERE username='%s' LIMIT 1", name.c_str());

        if (mysql_query(sql, order))
        {
            std::cerr << "Login query failed: " << mysql_error(sql) << std::endl;
            SqlConnPool::Instance()->FreeConn(sql);
            return false;
        }

        res = mysql_store_result(sql);
        if (!res)
        {
            std::cerr << "Failed to store result: " << mysql_error(sql) << std::endl;
            SqlConnPool::Instance()->FreeConn(sql);
            return false;
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        if (row)
        {
            std::string password(row[0]);
            if (pwd == password)
            {
                flag = true;
            }
        }
        mysql_free_result(res);
    }
    // 注册逻辑
    else
    {
        snprintf(order, 256, "SELECT username FROM user WHERE username='%s' LIMIT 1", name.c_str());

        if (mysql_query(sql, order))
        {
            std::cerr << "Register check query failed: " << mysql_error(sql) << std::endl;
            SqlConnPool::Instance()->FreeConn(sql);
            return false;
        }

        res = mysql_store_result(sql);
        if (!res)
        {
            std::cerr << "Failed to store result: " << mysql_error(sql) << std::endl;
            SqlConnPool::Instance()->FreeConn(sql);
            return false;
        }

        if (!mysql_fetch_row(res))
        { // 用户不存在，可以注册
            snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
            if (mysql_query(sql, order))
            {
                std::cerr << "User registration failed: " << mysql_error(sql) << std::endl;
                flag = false;
            }
            else
            {
                flag = true; // 注册成功
            }
        }
        else
        {
            std::cerr << "Username already exists." << std::endl;
            flag = false;
        }
        mysql_free_result(res);
    }

    SqlConnPool::Instance()->FreeConn(sql);
    return flag;
}

// 辅助函数：转换十六进制字符
int HttpRequest::ConverHex(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return -1;
}

// 获取请求路径
std::string HttpRequest::path() const
{
    return this->path_;
}

// 设置请求路径
void HttpRequest::path(const std::string &path)
{
    this->path_ = path;
}

// 获取请求方法（GET, POST等）
std::string HttpRequest::method() const
{
    return this->method_;
}

// 获取请求版本
std::string HttpRequest::version() const
{
    return this->version_;
}

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <sstream>
#include <mysql/mysql.h> // MySQL 连接池支持
#include "../buffer/Buffer.h"
#include "../pool/SqlConnRAII.h"
#include "../pool/SqlConnPool.h"


class HttpRequest
{
public:
    enum PARSE_STATE
    {
        REQUEST_LINE, // 请求行阶段
        HEADERS,      // 请求头阶段
        BODY,         // 请求体阶段
        FINISH,       // 请求完成
    };

    enum HTTP_CODE
    {
        NO_REQUEST = 0,    // 没有请求
        GET_REQUEST,       // GET 请求
        BAD_REQUEST,       // 错误请求
        NO_RESOURCE,       // 没有资源
        FORBIDDEN_REQUEST, // 禁止的请求
        FILE_REQUEST,      // 文件请求
        INTERNAL_ERROR,    // 内部错误
        CLOSED_CONNECTION, // 连接关闭
    };

    HttpRequest();
    ~HttpRequest() = default;

    // 初始化请求
    void Init();

    // 解析请求缓冲区
    bool parse(Buffer &buff);

    // 获取请求路径
    std::string path() const;

    // 设置请求路径
    void path(const std::string& path);

    // 获取请求方法（GET, POST等）
    std::string method() const;

    // 获取请求版本
    std::string version() const;

    // 获取POST请求表单中的参数
    std::string GetPost(const std::string &key) const;

    // 检查是否为长连接
    bool IsKeepAlive() const;

    // 请求路径处理
    void ParsePath_();

    // POST请求的表单参数处理
    void ParsePost_();

    // 解析URL编码的数据
    void ParseFromUrlencoded_();

    // 用户验证（例如登录）
    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

private:
    // 解析请求行
    bool ParseRequestLine_(const std::string &line);

    // 解析请求头
    void ParseHeader_(const std::string &line);

    // 解析请求体
    void ParseBody_(const std::string &line);

    // 解析POST表单数据
    void ParseFormData_();

    // 状态机的当前状态
    PARSE_STATE state_;

    // 请求方法、路径、版本、请求体
    std::string method_, path_, version_, body_;

    // 请求头和POST表单参数
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    // 存放默认HTML资源
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    // 辅助函数：转换十六进制字符
    static int ConverHex(char ch);
};

#endif // HTTP_REQUEST_H

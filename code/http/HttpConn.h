#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>   // readv/writev
#include <arpa/inet.h> // sockaddr_in
#include <stdlib.h>    // atoi()
#include <errno.h>
#include <iostream>
#include <atomic> // std::atomic
#include "../pool/SqlConnRAII.h"
#include "../buffer/Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    // 初始化连接
    void init(int sockFd, const sockaddr_in &addr);

    // 关闭连接
    void Close();

    // 读取数据
    ssize_t read(int *saveErrno);

    // 写入数据
    ssize_t write(int *saveErrno);

    // 获取文件描述符
    int GetFd() const;

    // 获取客户端端口
    int GetPort() const;

    // 获取客户端IP
    const char *GetIP() const;

    // 获取客户端地址
    sockaddr_in GetAddr() const;

    // 处理请求
    bool process();

    // 获取待写字节数
    int ToWriteBytes() const
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    // 判断是否保持长连接
    bool IsKeepAlive() const
    {
        return request_.IsKeepAlive();
    }

public:
    bool IsWriting() const { return isWriting_; }
    void SetWriting(bool flag) { isWriting_ = flag; }

private:
    bool isWriting_; // 表示是否在写数据中

    // 静态变量
    static const char *srcDir;         // 静态资源目录
    static std::atomic<int> userCount; // 活跃用户数

private:
    int fd_;           // 客户端文件描述符
    sockaddr_in addr_; // 客户端地址
    bool isClose_;     // 是否关闭连接

    int iovCnt_;          // iovec 计数
    struct iovec iov_[2]; // iovec 数组

    Buffer readBuff_;  // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;   // HTTP 请求对象
    HttpResponse response_; // HTTP 响应对象
};

#endif // HTTP_CONN_H

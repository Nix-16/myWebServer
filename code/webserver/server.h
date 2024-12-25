#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

#include "Epoll.h"
#include "../pool/SqlConnRAII.h"
#include "../pool/SqlConnPool.h"
#include "../buffer/Buffer.h"
#include "../http/HttpConn.h"
#include "../pool/ThreadPool.h"

class WebServer
{
public:
    WebServer(int port = 8080, int threadNum = 8);
    ~WebServer();

    void start();

private:
    void InitSocket_();                // 初始化服务器套接字
    void HandleListen_();              // 处理监听事件
    void HandleRead_(int fd);          // 处理读事件
    void HandleWrite_(int fd);         // 处理写事件
    void CloseConn_(HttpConn &client); // 关闭连接

    int port_;     // 监听端口
    int listenFd_; // 监听文件描述符
    bool isClose_; // 是否关闭服务器

    std::unique_ptr<Epoll> epoller_;          // epoll 管理器
    std::unordered_map<int, HttpConn> users_; // 客户端连接管理
    std::unique_ptr<ThreadPool> threadpool_;
};

#endif // WEBSERVER_H

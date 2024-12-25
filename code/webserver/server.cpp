#include "server.h"

// 构造函数：初始化成员变量
WebServer::WebServer(int port, int threadNum)
    : port_(port), isClose_(false)
{
    // 初始化数据库连接池
    SqlConnPool::Instance()->Init("localhost", 3306, "root", "6", "webserver", 6);

    // 初始化 epoll
    epoller_ = std::make_unique<Epoll>();

    // 初始化线程池
    threadpool_ = std::make_unique<ThreadPool>();

    // 初始化监听套接字
    InitSocket_();
}

// 析构函数：释放资源
WebServer::~WebServer()
{
    if (listenFd_ >= 0)
        close(listenFd_);
    isClose_ = true;
    SqlConnPool::Instance()->ClosePool();
}

// 启动服务器
void WebServer::start()
{
    while (!isClose_)
    {
        int eventCount = epoller_->Wait(0);
        for (int i = 0; i < eventCount; ++i)
        {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);

            if (fd == listenFd_)
            {
                HandleListen_(); // 处理新连接
            }
            else if (events & EPOLLIN)
            {
                // HandleRead_(fd); // 处理读事件
                threadpool_->addTask([this, fd]()
                                     { HandleRead_(fd); });
            }
            else if (events & EPOLLOUT)
            {
                // HandleWrite_(fd); // 处理写事件
                threadpool_->addTask([this, fd]()
                                     { HandleWrite_(fd); });
            }
            else
            {
                CloseConn_(users_[fd]); // 关闭连接
            }
        }
    }
}

// 初始化服务器套接字
void WebServer::InitSocket_()
{
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        exit(EXIT_FAILURE);
    }

    // 设置端口复用
    int optval = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (listen(listenFd_, 128) < 0)
    {
        exit(EXIT_FAILURE);
    }

    // 将监听套接字添加到 epoll
    epoller_->AddFd(listenFd_, EPOLLIN | EPOLLET);
    fcntl(listenFd_, F_SETFL, fcntl(listenFd_, F_GETFL) | O_NONBLOCK); // 设置非阻塞

}

// 处理新连接
void WebServer::HandleListen_()
{
    // std::cout << "handle listen" << std::endl;
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    while (true)
    {
        int clientFd = accept(listenFd_, (struct sockaddr *)&clientAddr, &len);

        if (clientFd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return; // 所有连接已处理
            }
            return;
        }

        if (users_.size() >= 20000)
        { // 最大连接数
            close(clientFd);
            return;
        }

        // 添加新连接
        users_[clientFd].init(clientFd, clientAddr);
        epoller_->AddFd(clientFd, EPOLLIN | EPOLLET | EPOLLONESHOT);
        fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL) | O_NONBLOCK);
    }
}

// 处理读事件
void WebServer::HandleRead_(int fd)
{
    int ret = -1;
    int err = 0;
    ret = users_[fd].read(&err);
    if (ret <= 0 && err != EAGAIN)
    {
        CloseConn_(users_[fd]);
        return;
    }
    if (users_[fd].process())
    {
        epoller_->ModFd(fd, EPOLLOUT | EPOLLET | EPOLLONESHOT);
    }
    else
    {
        epoller_->ModFd(fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
    }
}

// 处理写事件
void WebServer::HandleWrite_(int fd)
{
    int ret = -1;
    int err = 0;
    ret = users_[fd].write(&err);

    // std::cout << "[HandleWrite_] ToWriteBytes: " << users_[fd].ToWriteBytes()
    //           << ", IsKeepAlive: " << users_[fd].IsKeepAlive() << std::endl;

    if (users_[fd].ToWriteBytes() == 0)
    {
        if (users_[fd].IsKeepAlive())
        {
            if (users_[fd].process())
            {
                users_[fd].SetWriting(false);
                epoller_->ModFd(fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
            }
            return;
        }
        else
        {
            // std::cout << "close conn" << std::endl;
            CloseConn_(users_[fd]);
        }
        return;
    }

    // 写入未完成，缓冲区满
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        if (!users_[fd].IsWriting())
        {
            users_[fd].SetWriting(true); // 仅第一次标记写入状态
            // std::cout << "[HandleWrite_] First write incomplete, setting writing flag." << std::endl;
        }
        epoller_->ModFd(fd, EPOLLOUT | EPOLLET | EPOLLONESHOT);
        return;
    }

    CloseConn_(users_[fd]);
}

// 关闭连接
void WebServer::CloseConn_(HttpConn &client)
{
    epoller_->DelFd(client.GetFd());
    client.Close();
    users_.erase(client.GetFd());
}
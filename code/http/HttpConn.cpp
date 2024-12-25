#include "HttpConn.h"
#include <cassert>

// 静态变量初始化
const char *HttpConn::srcDir = "../resources";
std::atomic<int> HttpConn::userCount = 0;

HttpConn::HttpConn()
    : fd_(-1), isClose_(false), iovCnt_(0), isWriting_(false) {}

HttpConn::~HttpConn()
{
    Close();
}

void HttpConn::init(int sockFd, const sockaddr_in &addr)
{
    fd_ = sockFd;
    addr_ = addr;
    readBuff_.clear();
    writeBuff_.clear();
    isClose_ = false;
    userCount++;
}

void HttpConn::Close()
{
    if (!isClose_)
    {
        isClose_ = true;
        userCount--;
        if (fd_ >= 0)
        {
            close(fd_);
            fd_ = -1;
        }
    }
}

ssize_t HttpConn::read(int *saveErrno)
{
    ssize_t len = 0;
    while (true) // 一直读取，直到没有数据可读或发生错误
    {
        int len = readBuff_.ReadFd(fd_, saveErrno); // 读取数据
        if (len <= 0)
        {
            break; // 如果读取失败或没有数据可读，则退出循环
        }
    }
    return len; // 返回读取的总字节数，可能为 0 或负数，表示没有数据或发生错误
}

ssize_t HttpConn::write(int *saveErrno)
{
    ssize_t totalLen = 0; // 记录总共写入的字节数

    while (true)
    {
        ssize_t len = writev(fd_, iov_, iovCnt_); // 执行写操作

        if (len < 0)
        {
            *saveErrno = errno;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 缓冲区满，稍后重试
                //std::cerr << "Write temporarily unavailable (errno: " << errno << ")" << std::endl;
                break;
            }
            else
            {
                // 其他错误，打印日志并退出
                //std::cerr << "Write error: " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
                return -1;
            }
        }

        if (len == 0)
        {
            // 没有写入任何数据，退出循环
            break;
        }

        totalLen += len; // 累计写入的字节数

        // 处理 iovec 缓冲区
        if (len <= iov_[0].iov_len)
        {
            // 第一个缓冲区未完全写完
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
        }
        else
        {
            // 第一个缓冲区已写完，处理第二个缓冲区
            len -= iov_[0].iov_len;
            iov_[0].iov_len = 0;
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + len;
            iov_[1].iov_len -= len;
        }

        // 如果两个缓冲区都写完了，退出循环
        if (iov_[0].iov_len == 0 && iov_[1].iov_len == 0)
        {
            iovCnt_ = 0;
            break;
        }
    }

    return totalLen; // 返回总共写入的字节数
}

int HttpConn::GetFd() const
{
    return fd_;
}

int HttpConn::GetPort() const
{
    return ntohs(addr_.sin_port);
}

const char *HttpConn::GetIP() const
{
    return inet_ntoa(addr_.sin_addr);
}

sockaddr_in HttpConn::GetAddr() const
{
    return addr_;
}

bool HttpConn::process()
{
    //没有写才重置请求
    if(!IsWriting())
        request_.Init();

    if (readBuff_.readableBytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuff_))
    {
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else
    {
        response_.Init(srcDir, request_.path(), false, 400);
        return false;
    }

    response_.MakeResponse(writeBuff_);

    // 设置写缓冲区
    iov_[0].iov_base = const_cast<char *>(writeBuff_.peek());
    iov_[0].iov_len = writeBuff_.readableBytes();
    iovCnt_ = 1;

    if (response_.FileLen() > 0)
    {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    return true;
}

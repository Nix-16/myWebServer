#include "Epoll.h"
#include <unistd.h>
#include <stdexcept>
#include <cassert>

// 构造函数：创建 epoll 实例
Epoll::Epoll(int max_events)
    : max_events_(max_events), events_(max_events)
{
    epoll_fd_ = epoll_create(1);
    if (epoll_fd_ == -1)
    {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

// 析构函数：关闭 epoll 文件描述符
Epoll::~Epoll()
{
    if (epoll_fd_ >= 0)
        close(epoll_fd_);
}

// 添加文件描述符到 epoll 实例
void Epoll::AddFd(int fd, uint32_t events_mask)
{
    assert(fd >= 0);
    struct epoll_event ev
    {
    };
    ev.data.fd = fd;
    ev.events = events_mask;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        throw std::runtime_error("Failed to add fd to epoll");
    }
}

// 修改已经注册的文件描述符的事件
void Epoll::ModFd(int fd, uint32_t events_mask)
{
    struct epoll_event ev
    {
    };
    ev.data.fd = fd;
    ev.events = events_mask;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        throw std::runtime_error("Failed to modify fd in epoll");
    }
}

// 从 epoll 实例中移除文件描述符
void Epoll::DelFd(int fd)
{
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1)
    {
        throw std::runtime_error("Failed to remove fd from epoll");
    }
}

// 等待事件发生
int Epoll::Wait(int timeout)
{
    int event_count = epoll_wait(epoll_fd_, events_.data(), max_events_, timeout);
    if (event_count == -1)
    {
        throw std::runtime_error("epoll_wait error");
    }
    return event_count;
}

// 获取第 index 个事件的文件描述符
int Epoll::GetEventFd(int index) const
{
    if (index < 0 || index >= max_events_)
    {
        throw std::out_of_range("Index out of range in GetEventFd");
    }
    return events_[index].data.fd;
}

// 获取第 index 个事件的事件掩码
uint32_t Epoll::GetEvents(int index) const
{
    if (index < 0 || index >= max_events_)
    {
        throw std::out_of_range("Index out of range in GetEvents");
    }
    return events_[index].events;
}

// 索引操作符：访问事件列表
struct epoll_event &Epoll::operator[](int index)
{
    if (index < 0 || index >= max_events_)
    {
        throw std::out_of_range("Index out of range in operator[]");
    }
    return events_[index];
}

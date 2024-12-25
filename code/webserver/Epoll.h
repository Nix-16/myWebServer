#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <vector>
#include <stdexcept>

class Epoll {
public:
    // 构造函数，创建 epoll 实例
    explicit Epoll(int max_events = 1024);

    // 析构函数，关闭 epoll 文件描述符
    ~Epoll();

    // 注册文件描述符到 epoll 实例
    void AddFd(int fd, uint32_t events_mask);

    // 修改已经注册的文件描述符的事件
    void ModFd(int fd, uint32_t events_mask);

    // 从 epoll 实例中移除文件描述符
    void DelFd(int fd);

    // 等待并返回发生的事件数量
    int Wait(int timeout = -1);

    // 获取第 index 个事件的文件描述符
    int GetEventFd(int index) const;

    // 获取第 index 个事件的事件掩码
    uint32_t GetEvents(int index) const;

    // 索引操作符，访问事件列表（备用接口）
    struct epoll_event &operator[](int index);

private:
    int epoll_fd_;                           // epoll 文件描述符
    int max_events_;                         // 最大事件数
    std::vector<struct epoll_event> events_; // 存储事件的向量
};

#endif // EPOLL_H

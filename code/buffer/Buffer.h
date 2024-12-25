#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <cassert>
#include <cstring> // for memcpy

class Buffer
{
public:
    explicit Buffer(size_t initial_size = 1024); // 构造函数，初始缓冲区大小

    // 可写区域的大小
    size_t writableBytes() const;

    // 可读区域的大小
    size_t readableBytes() const;

    // 已读区域的大小
    size_t prependableBytes() const;

    // 将数据追加到缓冲区
    void append(const std::string &data);
    void append(const char *data, size_t len);

    // 从缓冲区中提取指定长度的数据
    std::string retrieve(size_t len);

    // 从缓冲区中提取所有数据
    std::string retrieveAll();

    // 清空缓冲区
    void clear();

    // 获取当前缓冲区可读数据的起始地址
    const char *peek() const;

    // 获取当前缓冲区写指针的地址
    char *beginWrite();
    const char *beginWrite() const; // Const 版本

    // 确保缓冲区有足够的空间写入数据
    void ensureWritableBytes(size_t len);

    // 从文件描述符（例如套接字）读取数据并将其存储到缓冲区
    ssize_t ReadFd(int fd, int *Errno);
    ssize_t WriteFd(int fd, int *Errno);


    std::vector<char> buffer_; // 实际存储数据的缓冲区

private:
    //std::vector<char> buffer_; // 实际存储数据的缓冲区
    size_t reader_index_;      // 读指针位置
    size_t writer_index_;      // 写指针位置

    // 扩展缓冲区以容纳更多数据
    void makeSpace(size_t len);

    // 返回缓冲区起始地址
    char *begin();
    const char *begin() const;
};

#endif // BUFFER_H

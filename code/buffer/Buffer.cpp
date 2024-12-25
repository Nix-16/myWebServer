#include "Buffer.h"
#include <unistd.h>  // for read(), write()
#include <sys/uio.h> // for struct iovec
#include <errno.h>   // for errno

// 构造函数，初始化缓冲区大小
Buffer::Buffer(size_t initial_size)
    : buffer_(initial_size), reader_index_(0), writer_index_(0) {}

// 返回可写区域的大小
size_t Buffer::writableBytes() const
{
    return buffer_.size() - writer_index_;
}

// 返回可读区域的大小
size_t Buffer::readableBytes() const
{
    return writer_index_ - reader_index_;
}

// 返回已读区域的大小
size_t Buffer::prependableBytes() const
{
    return reader_index_;
}

// 将数据追加到缓冲区
void Buffer::append(const std::string &data)
{
    append(data.c_str(), data.size());
}

void Buffer::append(const char *data, size_t len)
{
    ensureWritableBytes(len);
    std::memcpy(begin() + writer_index_, data, len);
    writer_index_ += len;
}

// 从缓冲区中提取指定长度的数据
std::string Buffer::retrieve(size_t len)
{
    assert(len <= readableBytes());
    std::string result(peek(), len);
    reader_index_ += len;
    if (reader_index_ == writer_index_)
    {
        clear(); // 如果全部读取，重置缓冲区
    }
    return result;
}

// 从缓冲区中提取所有数据
std::string Buffer::retrieveAll()
{
    return retrieve(readableBytes());
}

// 清空缓冲区
void Buffer::clear()
{
    reader_index_ = 0;
    writer_index_ = 0;
}

// 获取当前缓冲区可读数据的起始地址
const char *Buffer::peek() const
{
    return begin() + reader_index_;
}

// 获取当前缓冲区写指针的地址

char *Buffer::beginWrite()
{
    return &*buffer_.begin() + writer_index_;
}

const char *Buffer::beginWrite() const
{
    return &*buffer_.begin() + writer_index_;
}

// 确保缓冲区有足够的空间写入数据
void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() < len)
    {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}

// 扩展缓冲区以容纳更多数据
void Buffer::makeSpace(size_t len)
{
    if (writableBytes() + prependableBytes() < len)
    {
        buffer_.resize(writer_index_ + len);
    }
    else
    {
        size_t readable = readableBytes();
        std::memmove(begin(), peek(), readable);
        reader_index_ = 0;
        writer_index_ = readable;
    }
}

// 返回缓冲区的起始地址
char *Buffer::begin()
{
    return &*buffer_.begin();
}

const char *Buffer::begin() const
{
    return &*buffer_.begin();
}

// 从文件描述符读取数据并存储到缓冲区
ssize_t Buffer::ReadFd(int fd, int *Errno)
{
    char extra_buffer[65536]; // 临时缓冲区，用于防止空间不足
    struct iovec iov[2];
    size_t writable = writableBytes();

    // 第一块缓冲区：指向当前缓冲区的可写区域
    iov[0].iov_base = begin() + writer_index_;
    iov[0].iov_len = writable;

    // 第二块缓冲区：临时缓冲区
    iov[1].iov_base = extra_buffer;
    iov[1].iov_len = sizeof(extra_buffer);

    // 读取数据
    ssize_t n = readv(fd, iov, 2);
    if (n < 0)
    {
        *Errno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        writer_index_ += n;
    }
    else
    {
        writer_index_ = buffer_.size();
        append(extra_buffer, n - writable); // 将剩余数据写入缓冲区
    }
    return n;
}

// 将缓冲区数据写入文件描述符
ssize_t Buffer::WriteFd(int fd, int *Errno)
{
    size_t readable = readableBytes();
    ssize_t n = write(fd, peek(), readable);
    if (n < 0)
    {
        *Errno = errno;
    }
    else
    {
        reader_index_ += n;
        if (reader_index_ == writer_index_)
        {
            clear(); // 如果写完所有数据，重置缓冲区
        }
    }
    return n;
}

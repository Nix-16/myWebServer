#include "SqlConnPool.h"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <algorithm>
#include <iostream>

SqlConnPool::SqlConnPool()
    : MAX_CONN_(0), useCount_(0), freeCount_(0)
{
    sem_init(&semId_, 0, 0); // 初始化信号量
}

SqlConnPool *SqlConnPool::Instance()
{
    static SqlConnPool instance;
    return &instance;
}

// 初始化连接池
void SqlConnPool::Init(const char *host, int port,
                       const char *user, const char *pwd,
                       const char *dbName, int connSize)
{
    if (connSize <= 0 || host == nullptr || user == nullptr || pwd == nullptr || dbName == nullptr)
    {
        std::cerr << "Invalid database connection pool parameters." << std::endl;
        return;
    }

    // std::cout << "MySQL Connection Parameters:" << std::endl;
    // std::cout << "Host: " << (host ? host : "NULL") << std::endl;
    // std::cout << "Port: " << port << std::endl;
    // std::cout << "User: " << (user ? user : "NULL") << std::endl;
    // std::cout << "Password: " << (pwd ? pwd : "NULL") << std::endl;
    // std::cout << "Database Name: " << (dbName ? dbName : "NULL") << std::endl;
    // std::cout << "Connection Pool Size: " << connSize << std::endl;

    MAX_CONN_ = connSize; // 先设置最大连接数

    for (int i = 0; i < MAX_CONN_; ++i)
    {
        MYSQL *conn = createConn(host, port, user, pwd, dbName);
        if (conn)
        {
            connQue_.push(conn);
            ++freeCount_;
        }
        else
        {
            std::cerr << "Failed to create MySQL connection at index " << i << std::endl;
        }
    }

    sem_init(&semId_, 0, MAX_CONN_);
}

// 创建数据库连接
MYSQL *SqlConnPool::createConn(const char *host, int port,
                               const char *user, const char *pwd,
                               const char *dbName)
{
    MYSQL *conn = mysql_init(nullptr);
    if (!conn || !mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0))
    {
        std::cerr << "MySQL connection error: " << mysql_error(conn) << std::endl;
        return nullptr;
    }
    return conn;
}

// 获取连接（支持超时等待）
MYSQL *SqlConnPool::GetConn(int timeout_ms)
{
    MYSQL *sql = nullptr;

    if (sem_wait(&semId_) != 0)
    {
        std::cerr << "Failed to acquire semaphore." << std::endl;
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!connQue_.empty())
        {
            sql = connQue_.front();
            connQue_.pop();
            --freeCount_;
            ++useCount_;
        }
    }

    return sql;
}

// 释放连接
void SqlConnPool::FreeConn(MYSQL *conn)
{
    if (!conn)
        return;

    std::lock_guard<std::mutex> lock(mtx_);
    connQue_.push(conn);
    ++freeCount_;
    --useCount_;
    sem_post(&semId_);
}

// 关闭连接池
void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> lock(mtx_);
    while (!connQue_.empty())
    {
        MYSQL *conn = connQue_.front();
        connQue_.pop();
        closeConn(conn);
    }

    sem_destroy(&semId_);
}

// 关闭单个连接
void SqlConnPool::closeConn(MYSQL *conn)
{
    if (conn)
    {
        mysql_close(conn); // 只需关闭连接，无需 delete
    }
}

// 获取空闲连接数
int SqlConnPool::GetFreeConnCount() const
{
    return freeCount_;
}

// 析构函数
SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

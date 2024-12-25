#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <atomic>
#include <thread>
#include <iostream>

class SqlConnPool
{
public:
    static SqlConnPool *Instance();

    MYSQL *GetConn(int timeout_ms = 1000); // 连接获取，支持超时
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount() const;

    void Init(const char *host, int port,
              const char *user, const char *pwd,
              const char *dbName, int connSize);
    void ClosePool();
    ~SqlConnPool();

private:
    SqlConnPool();
    MYSQL *createConn(const char *host, int port,
                      const char *user, const char *pwd,
                      const char *dbName); // 创建新连接

    void closeConn(MYSQL *conn); // 关闭连接并清理资源


    int MAX_CONN_;  // 最大连接数
    int useCount_;  // 当前使用的连接数
    int freeCount_; // 当前空闲的连接数

    std::queue<MYSQL *> connQue_;  // 存放空闲连接的队列
    mutable std::mutex mtx_;       // 保护 connQue_ 的线程安全
    sem_t semId_;                  // 信号量控制可用连接数
};

#endif // SQLCONNPOOL_H

#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <future>
#include <iostream>

class ThreadPool
{
public:
    ThreadPool(int min = 4, int max = std::thread::hardware_concurrency());
    ~ThreadPool();

    // 添加任务，支持返回值
    // ThreadPool.h

    template <typename Func, typename... Args>
    auto addTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        using ReturnType = decltype(func(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_tasks.emplace([task]()
                            { (*task)(); });
        }

        m_condition.notify_one();
        return task->get_future();
    }

private:
    void manager(); // 管理者线程
    void worker();  // 工作线程

private:
    std::thread m_managerThread;        // 管理者线程
    std::vector<std::thread> m_workers; // 工作线程集合

    std::atomic<bool> m_stop;       // 线程池是否停止
    std::atomic<int> m_curThreads;  // 当前线程数
    std::atomic<int> m_idleThreads; // 空闲线程数

    std::queue<std::function<void()>> m_tasks; // 任务队列
    std::mutex m_queueMutex;                   // 任务队列互斥锁
    std::condition_variable m_condition;       // 条件变量
    const int m_maxThreads;                    // 最大线程数
    const int m_minThreads;                    // 最小线程数
};
#endif
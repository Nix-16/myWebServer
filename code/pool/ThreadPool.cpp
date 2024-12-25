#include "ThreadPool.h"

// 构造函数
ThreadPool::ThreadPool(int minThreads, int maxThreads )
    : m_minThreads(minThreads),
      m_maxThreads(maxThreads),
      m_stop(false),
      m_curThreads(0),
      m_idleThreads(0)
{

    // 初始化最小线程数
    for (int i = 0; i < m_minThreads; ++i)
    {
        m_workers.emplace_back(&ThreadPool::worker, this);
        ++m_curThreads;
        ++m_idleThreads;
    }

    // 启动管理线程
    m_managerThread = std::thread(&ThreadPool::manager, this);

    std::cout << "ThreadPool initialized with " << m_minThreads << " threads." << std::endl;
}

// 析构函数
ThreadPool::~ThreadPool()
{
    m_stop = true;
    m_condition.notify_all();

    // 等待所有工作线程退出
    for (auto &thread : m_workers)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 等待管理线程退出
    if (m_managerThread.joinable())
    {
        m_managerThread.join();
    }

    //std::cout << "ThreadPool destroyed." << std::endl;
}

// 工作线程函数
void ThreadPool::worker()
{
    while (!m_stop)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this]()
                             { return m_stop || !m_tasks.empty(); });

            if (m_stop && m_tasks.empty())
            {
                break;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
            --m_idleThreads;
        }

        // 执行任务
        try
        {
            task();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception in task: " << e.what() << std::endl;
        }

        ++m_idleThreads;
    }
}

// 管理线程函数
void ThreadPool::manager()
{
    while (!m_stop)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 每5秒检查一次

        if (m_tasks.size() > m_idleThreads && m_curThreads < m_maxThreads)
        {
            // 增加线程
            m_workers.emplace_back(&ThreadPool::worker, this);
            ++m_curThreads;
            ++m_idleThreads;
            //std::cout << "Added new thread. Current threads: " << m_curThreads << std::endl;
        }

        if (m_idleThreads > m_minThreads && m_curThreads > m_minThreads)
        {
            // 减少线程（让空闲线程自我终止）
            --m_curThreads;
            --m_idleThreads;
            //std::cout << "Removed one thread. Current threads: " << m_curThreads << std::endl;
        }
    }
}

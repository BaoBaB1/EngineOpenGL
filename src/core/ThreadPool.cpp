#include "ThreadPool.hpp"

namespace fury
{
  ThreadPool::ThreadPool(unsigned int thread_count)
  {
    for (unsigned int i = 0; i < thread_count; i++)
    {
      m_workers.emplace_back([this]() { worker_job(); });
    }
  }

  void ThreadPool::worker_job()
  {
    while (true)
    {
      Task task;
      {
        std::unique_lock ul(m_queue_mutex);
        m_cv.wait(ul, [this]() { return !m_queue.empty() || m_stop; });
        if (m_stop && m_queue.empty())
        {
          return;
        }
        task = std::move(m_queue.front());
        m_queue.pop();
      }
      task();
    }
  }

  ThreadPool::~ThreadPool()
  {
    m_stop = true;
    m_cv.notify_all();
    for (auto& worker : m_workers)
    {
      worker.join();
    }
  }

} // namespace fury
#pragma once

#include <thread>
#include <future>
#include <type_traits>
#include <queue>
#include <condition_variable>
#include <mutex>

namespace fury
{
  class ThreadPool
  {
  public:
    using Task = std::packaged_task<void()>;
    static ThreadPool& instance()
    {
      static ThreadPool tp;
      return tp;
    }
    ThreadPool(unsigned int thread_count = std::thread::hardware_concurrency());
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = default;
    ThreadPool& operator=(ThreadPool&&) = default;
    ~ThreadPool();

    template<typename F>
    std::future<std::invoke_result_t<F>> enqueue(F&& func)
    {
      using R = std::invoke_result_t<F>;
      auto sp_task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(func));
      auto fut = sp_task->get_future();
      Task wrapped_task([sp_task]() { (*sp_task)(); });
      {
        std::lock_guard lg(m_queue_mutex);
        m_queue.push(std::move(wrapped_task));
      }
      m_cv.notify_one();
      return fut;
    }
    unsigned int get_workers_count() const { return static_cast<unsigned int>(m_workers.size()); }

  private:
    void worker_job();
  private:
    std::vector<std::thread> m_workers;
    std::queue<Task> m_queue;
    std::condition_variable m_cv;
    std::mutex m_queue_mutex;
    bool m_stop = false;
  };
} // namespace fury

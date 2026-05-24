#include "core/ThreadPool.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <unordered_set>

using namespace fury;
using namespace std::chrono;
using namespace std::chrono_literals;

TEST(ThreadPoolTest, CreateThreadPool)
{
  ThreadPool tp(4);
  EXPECT_EQ(tp.get_workers_count(), 4);
}

TEST(ThreadPoolTest, EnqueueMaxConcurrentThreads)
{
  ThreadPool tp;
  const int N = std::thread::hardware_concurrency();
  std::vector<std::thread::id> thread_ids(N);
  std::vector<std::future<int>> futures;
  for (int i = 0; i < N; i++)
  {
    std::thread::id& id = thread_ids[i];
    futures.emplace_back(tp.enqueue(
        [i = i + 1, &id]()
        {
          id = std::this_thread::get_id();
          std::this_thread::sleep_for(1s);
          return i * i;
        }));
  }
  for (int i = 1; auto& future : futures)
  {
    EXPECT_EQ(future.get(), i * i);
    ++i;
  }
  std::unordered_set<std::thread::id> unique_thread_ids(thread_ids.begin(), thread_ids.end());
  EXPECT_TRUE(unique_thread_ids.size() == N);
}

TEST(ThreadPoolTest, AllTasksFinishedAfterDestruction)
{
  const int workers = std::max(std::thread::hardware_concurrency() / 2, 1u);
  const int tasks = workers * 3;
  std::vector<std::future<int>> futures;
  {
    ThreadPool tp(workers);
    for (int i = 0; i < tasks; i++)
    {
      futures.emplace_back(tp.enqueue(
          [i = i + 1]()
          {
            std::this_thread::sleep_for(0.5s);
            return i * i;
          }));
    }
  }
  EXPECT_EQ(futures.size(), tasks);
  for (int i = 1; auto& future : futures)
  {
    EXPECT_EQ(future.get(), i * i);
    ++i;
  }
}

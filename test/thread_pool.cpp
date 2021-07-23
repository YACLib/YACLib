#include "util/cpu_time.hpp"
#include "util/time.hpp"

#include <yaclib/executor/thread_pool.hpp>

#include <atomic>
#include <iostream>
#include <thread>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;
using namespace std::chrono_literals;

GTEST_TEST(single_threaded, just_works) {
  auto tp = executor::MakeThreadPool();

  bool ready{false};
  tp->Execute([&ready] {
    ready = true;
  });
  tp->Stop();
  tp->Wait();
  EXPECT_TRUE(ready);
}

GTEST_TEST(single_threaded, exception) {
  auto tp = executor::MakeThreadPool(1);

  bool ready{false};
  tp->Execute([&ready] {
    ready = true;
    throw std::runtime_error("task failed");
  });
  tp->Stop();
  tp->Wait();
  EXPECT_TRUE(ready);
}

GTEST_TEST(single_threaded, many_tasks) {
  auto tp = executor::MakeThreadPool(1);
  static const size_t kTasks = 17;
  std::atomic<size_t> tasks{0};

  for (size_t i = 0; i < kTasks; ++i) {
    tp->Execute([&] {
      ++tasks;
    });
  }
  tp->Stop();
  tp->Wait();

  EXPECT_EQ(tasks.load(), kTasks);
}

GTEST_TEST(multithreaded, simple) {
  auto tp = executor::MakeThreadPool(4);

  std::atomic<size_t> tasks{0};

  tp->Execute([&] {
    std::this_thread::sleep_for(1s);
    ++tasks;
  });

  tp->Execute([&] {
    ++tasks;
  });

  std::this_thread::sleep_for(100ms);

  ASSERT_EQ(tasks.load(), 1);

  tp->Stop();
  tp->Wait();

  ASSERT_EQ(tasks.load(), 2);
}

GTEST_TEST(two_pools, simple) {
  auto factory = executor::MakeThreadFactory(4);
  auto pool1 = executor::MakeThreadPool(2, factory);
  auto pool2 = executor::MakeThreadPool(2, factory);

  std::atomic<size_t> tasks{0};
  util::StopWatch stop_watch;

  pool1->Execute([&] {
    std::this_thread::sleep_for(1s);
    tasks.fetch_add(1, std::memory_order_relaxed);
  });

  pool2->Execute([&] {
    std::this_thread::sleep_for(1s);
    tasks.fetch_add(1, std::memory_order_relaxed);
  });

  pool1->Stop();
  pool2->Stop();
  pool1->Wait();
  pool2->Wait();

  EXPECT_TRUE(stop_watch.Elapsed() < 1500ms);
  EXPECT_EQ(tasks.load(), 2);
}

GTEST_TEST(stop, simple) {
  auto pool = executor::MakeThreadPool(3);

  util::StopWatch stop_watch;

  for (size_t i = 0; i < 3; ++i) {
    pool->Execute([]() {
      std::this_thread::sleep_for(1s);
    });
  }

  for (size_t i = 0; i < 10; ++i) {
    pool->Execute([]() {
      std::this_thread::sleep_for(1s);
    });
  }
  std::this_thread::sleep_for(250ms);
  pool->Stop();
  EXPECT_LE(stop_watch.Elapsed(), 1s);
}

/* TODO(Ri7ay): Dont work on windows, check this:
 *   -
 * https://stackoverflow.com/questions/12606033/computing-cpu-time-in-c-on-windows
 */
#if __linux
GTEST_TEST(simple, dont_burn_cpu) {
  auto pool = executor::MakeThreadPool(4);

  // Warmup
  for (size_t i = 0; i < 4; ++i) {
    pool->Execute([&]() {
      std::this_thread::sleep_for(100ms);
    });
  }

  util::ProcessCPUTimer cpu_timer;

  std::this_thread::sleep_for(1s);

  pool->Stop();
  pool->Wait();

  EXPECT_TRUE(cpu_timer.Elapsed() < 100ms);
}
#endif

GTEST_TEST(simple, current) {
  auto tp = executor::MakeThreadPool(4);

  EXPECT_EQ(executor::CurrentThreadPool(), nullptr);

  tp->Execute([&] {
    EXPECT_EQ(executor::CurrentThreadPool(), tp.get());
  });

  tp->Stop();
  tp->Wait();
}

GTEST_TEST(simple, execute_after_join) {
  auto tp = executor::MakeThreadPool(4);

  bool done = false;

  tp->Execute([&] {
    std::this_thread::sleep_for(500ms);

    executor::CurrentThreadPool()->Execute([&] {
      std::this_thread::sleep_for(500ms);
      done = true;
    });

    tp->Stop();
  });

  tp->Wait();

  EXPECT_TRUE(done);
}

GTEST_TEST(simple, execute_after_cancel) {
  auto tp = executor::MakeThreadPool(4);

  bool done = false;

  tp->Execute([&] {
    std::this_thread::sleep_for(500ms);
    executor::CurrentThreadPool()->Execute([&] {
      std::this_thread::sleep_for(500ms);
      done = true;
    });
  });

  tp->HardStop();
  tp->Wait();
  EXPECT_FALSE(done);
}

GTEST_TEST(simple, racy) {
  auto tp = executor::MakeThreadPool(4);

  std::atomic<int> shared_counter{0};
  std::atomic<int> tasks{0};

  for (size_t i = 0; i < 100500; ++i) {
    tp->Execute([&] {
      int old = shared_counter.load();
      shared_counter.store(old + 1);

      ++tasks;
    });
  }

  tp->Stop();
  tp->Wait();

  std::cout << "Racy counter value: " << shared_counter << std::endl;

  EXPECT_EQ(tasks.load(), 100500);
  EXPECT_LE(shared_counter.load(), 100500);
}

GTEST_TEST(simple, test_lifetime) {
  auto tp = executor::MakeThreadPool(4);

  std::atomic<int> dead{0};

  class Task {
   public:
    Task(std::atomic<int>& done) : counter_(done) {
    }
    Task(const Task&) = delete;
    Task(Task&&) = default;

    ~Task() {
      if (done_) {
        counter_.fetch_add(1);
      }
    }

    void operator()() {
      std::this_thread::sleep_for(100ms);
      done_ = true;
    }

   private:
    bool done_{false};
    std::atomic<int>& counter_;
  };

  for (int i = 0; i < 4; ++i) {
    tp->Execute(Task(dead));
  }
  std::this_thread::sleep_for(500ms);
  EXPECT_EQ(dead.load(), 4);
  tp->Stop();
  tp->Wait();
}

GTEST_TEST(simple, use_threads) {
  util::StopWatch clock;

  auto pool = executor::MakeThreadPool(4);

  std::atomic<size_t> tasks{0};

  for (size_t i = 0; i < 4; ++i) {
    pool->Execute([&] {
      std::this_thread::sleep_for(750ms);
      ++tasks;
    });
  }

  pool->Stop();
  pool->Wait();

  EXPECT_EQ(tasks.load(), 4);
  EXPECT_LE(clock.Elapsed(), 1s);
}

}  // namespace

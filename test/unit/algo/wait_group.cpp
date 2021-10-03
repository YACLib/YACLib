#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <chrono>

#include <gtest/gtest.h>

using namespace yaclib;
using namespace std::chrono_literals;

TEST(Wait, JustWorks) {
  auto tp = executor::MakeThreadPool(1);
  auto [f, p] = async::MakeContract<void>();

  test::util::StopWatch timer;

  tp->Execute([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(150ms);
    std::move(p).Set();
  });
  algo::Wait(f);
  EXPECT_TRUE(f.Ready());
  EXPECT_LE(timer.Elapsed(), 200ms);
  tp->Stop();
  tp->Wait();
}

TEST(WaitFor, JustWorks) {
  auto tp = executor::MakeThreadPool(1);
  auto [f, p] = async::MakeContract<void>();

  test::util::StopWatch timer;

  tp->Execute([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(150ms);
    std::move(p).Set();
  });
  algo::WaitFor(50ms, f);
  EXPECT_TRUE(!f.Ready());
  EXPECT_LE(timer.Elapsed(), 100ms);
  tp->Stop();
  tp->Wait();
}

TEST(WaitUntil, JustWorks) {
  auto tp = executor::MakeThreadPool(1);
  auto [f, p] = async::MakeContract<void>();

  test::util::StopWatch timer;
  tp->Execute([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(150ms);
    std::move(p).Set();
  });
  algo::WaitUntil(timer.Now() + 50ms, f);
  EXPECT_TRUE(!f.Ready());
  EXPECT_LE(timer.Elapsed(), 100ms);
  tp->Stop();
  tp->Wait();
}

TEST(Wait, Multithreaded) {
  static constexpr int kThreads = 4;
  auto tp = executor::MakeThreadPool(kThreads);
  async::Future<int> f[kThreads];
  for (int i = 0; i < kThreads; ++i) {
    f[i] = async::Run(tp, [i] {
      std::this_thread::sleep_for(50ms);
      return i;
    });
  }
  test::util::StopWatch timer;
  algo::Wait(f[0], f[1], f[2], f[3]);
  EXPECT_LE(timer.Elapsed(), 100ms);
  EXPECT_TRUE(std::all_of(std::begin(f), std::end(f), [](auto& f) {
    return f.Ready();
  }));
  for (int i = 0; i < kThreads; ++i) {
    EXPECT_EQ(std::move(f[i]).Get().Value(), i);
  }
  tp->Stop();
  tp->Wait();
}

TEST(WaitFor, Multithreaded) {
  static constexpr int kThreads = 4;
  auto tp = executor::MakeThreadPool(2);
  async::Future<void> f[kThreads];
  for (size_t i = 0; i < kThreads; ++i) {
    f[i] = async::Run(tp, [] {
    });
  }
  algo::Wait(f[0], f[1], f[2], f[3]);
  for (size_t i = 0; i < kThreads; ++i) {
    EXPECT_TRUE(f[i].Ready());
  }
  tp->Stop();
  tp->Wait();
}

TEST(WaitUntil, Multithreaded) {
}

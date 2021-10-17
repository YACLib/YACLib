#include <util/time.hpp>

#include <yaclib/algo/wait.hpp>
#include <yaclib/algo/wait_for.hpp>
#include <yaclib/algo/wait_until.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <chrono>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

enum class WaitPolicy {
  Endless,
  For,
  Until,
};

template <WaitPolicy policy>
void TestJustWorks() {
  auto tp = yaclib::MakeThreadPool(1);
  auto [f, p] = yaclib::MakeContract<void>();

  test::util::StopWatch timer;

  tp->Execute([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
    std::move(p).Set();
  });
  if constexpr (policy == WaitPolicy::Endless) {
    yaclib::Wait(f);
    EXPECT_TRUE(f.Ready());
    EXPECT_LE(timer.Elapsed(), 200ms * YACLIB_CI_SLOWDOWN);
  } else if constexpr (policy == WaitPolicy::For) {
    yaclib::WaitFor(50ms * YACLIB_CI_SLOWDOWN, f);
    EXPECT_TRUE(!f.Ready());
    EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  } else if constexpr (policy == WaitPolicy::Until) {
    yaclib::WaitUntil(timer.Now() + 50ms * YACLIB_CI_SLOWDOWN, f);
    EXPECT_TRUE(!f.Ready());
    EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  }

  tp->Stop();
  tp->Wait();
}

TEST(Wait, JustWorks) {
  TestJustWorks<WaitPolicy::Endless>();
}

TEST(WaitFor, JustWorks) {
  TestJustWorks<WaitPolicy::For>();
}

TEST(WaitUntil, JustWorks) {
  TestJustWorks<WaitPolicy::Until>();
}

template <WaitPolicy kPolicy>
void TestMultithreaded() {
  static constexpr int kThreads = 4;
  auto tp = yaclib::MakeThreadPool(kThreads);
  yaclib::Future<int> f[kThreads];

  for (int i = 0; i < kThreads; ++i) {
    f[i] = yaclib::Run(tp, [i] {
      std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return i;
    });
  }
  test::util::StopWatch timer;
  if constexpr (kPolicy == WaitPolicy::Endless) {
    yaclib::Wait(f[0], f[1], f[2], f[3]);
    EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  } else if constexpr (kPolicy == WaitPolicy::For) {
    bool ready = yaclib::WaitFor(100ms * YACLIB_CI_SLOWDOWN, f[0], f[1], f[2], f[3]);
    EXPECT_TRUE(ready);
  } else if constexpr (kPolicy == WaitPolicy::Until) {
    bool ready = yaclib::WaitUntil(timer.Now() + 100ms * YACLIB_CI_SLOWDOWN, f[0], f[1], f[2], f[3]);
    EXPECT_TRUE(ready);
  }
  EXPECT_LE(timer.Elapsed(), 150ms * YACLIB_CI_SLOWDOWN);

  EXPECT_TRUE(std::all_of(std::begin(f), std::end(f), [](auto& f) {
    return f.Ready();
  }));

  for (int i = 0; i < kThreads; ++i) {
    EXPECT_EQ(std::move(f[i]).Get().Value(), i);
  }
  tp->Stop();
  tp->Wait();
}

TEST(Wait, Multithreaded) {
  TestMultithreaded<WaitPolicy::Endless>();
}

TEST(WaitFor, Multithreaded) {
  TestMultithreaded<WaitPolicy::For>();
}

TEST(WaitUntil, Multithreaded) {
  TestMultithreaded<WaitPolicy::Until>();
}

template <WaitPolicy kPolicy>
void TestHaveResults() {
  auto [f1, p1] = yaclib::MakeContract<void>();
  auto [f2, p2] = yaclib::MakeContract<void>();
  auto tp = yaclib::MakeThreadPool(1);
  tp->Execute([p1 = std::move(p1), p2 = std::move(p2)]() mutable {
    std::move(p1).Set();
    std::move(p2).Set();
  });
  test::util::StopWatch timer;
  std::this_thread::sleep_for(100ms * YACLIB_CI_SLOWDOWN);
  if constexpr (kPolicy == WaitPolicy::Endless) {
    yaclib::Wait(f1, f2);
  } else if constexpr (kPolicy == WaitPolicy::Until) {
    yaclib::WaitUntil(timer.Now() + 10ms, f1, f2);
  } else {
    yaclib::WaitFor(10ms, f1, f2);
  }
  EXPECT_TRUE(f1.Ready());
  EXPECT_TRUE(f2.Ready());
}

TEST(Wait, HaveResults) {
  TestHaveResults<WaitPolicy::Endless>();
}

TEST(WaitFor, HaveResults) {
  TestHaveResults<WaitPolicy::For>();
}

TEST(WaitUntil, HaveResults) {
  TestHaveResults<WaitPolicy::Until>();
}

#include <util/time.hpp>

#include <yaclib/algo/wait.hpp>
#include <yaclib/algo/wait_for.hpp>
#include <yaclib/algo/wait_until.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <chrono>

#include <gtest/gtest.h>

using namespace yaclib;
using namespace std::chrono_literals;

enum class WaitPolicy {
  Endless,
  For,
  Until,
};

template <WaitPolicy policy>
void TestJustWorks() {
  auto tp = MakeThreadPool(1);
  auto [f, p] = MakeContract<void>();

  test::util::StopWatch timer;

  tp->Execute([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
    std::move(p).Set();
  });
  if constexpr (policy == WaitPolicy::Endless) {
    Wait(f);
    EXPECT_TRUE(f.Ready());
    EXPECT_LE(timer.Elapsed(), 200ms * YACLIB_CI_SLOWDOWN);
  } else if constexpr (policy == WaitPolicy::For) {
    WaitFor(50ms * YACLIB_CI_SLOWDOWN, f);
    EXPECT_TRUE(!f.Ready());
    EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  } else if constexpr (policy == WaitPolicy::Until) {
    WaitUntil(timer.Now() + 50ms * YACLIB_CI_SLOWDOWN, f);
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
  auto tp = MakeThreadPool(kThreads);
  Future<int> f[kThreads];

  for (int i = 0; i < kThreads; ++i) {
    f[i] = Run(tp, [i] {
      std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return i;
    });
  }
  test::util::StopWatch timer;
  if constexpr (kPolicy == WaitPolicy::Endless) {
    Wait(f[0], f[1], f[2], f[3]);
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

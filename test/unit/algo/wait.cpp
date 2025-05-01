#include <util/time.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/wait.hpp>
#include <yaclib/async/wait_for.hpp>
#include <yaclib/async/wait_until.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <ratio>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/chrono>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

enum class WaitPolicy {
  Endless,
  For,
  Until,
};

template <WaitPolicy policy>
void TestJustWorks() {
  yaclib::FairThreadPool tp{1};
  auto [f, p] = yaclib::MakeContract<>();

  test::util::StopWatch timer;

  Submit(tp, [p = std::move(p)]() mutable {
    yaclib_std::this_thread::sleep_for(150ms * YACLIB_CI_SLOWDOWN);
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
    WaitUntil(yaclib_std::chrono::system_clock::now() + 50ms * YACLIB_CI_SLOWDOWN, f);
    EXPECT_TRUE(!f.Ready());
    EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  }

  tp.Stop();
  tp.Wait();
}

TEST(Wait, Empty) {
  std::vector<yaclib::Future<int>> fs;

  Wait(fs.begin(), 0);
  Wait(fs.begin(), fs.end());

  EXPECT_TRUE(WaitFor(0ns, fs.begin(), 0));
  EXPECT_TRUE(WaitFor(0ns, fs.begin(), fs.end()));

  EXPECT_TRUE(WaitUntil(yaclib_std::chrono::steady_clock::now(), fs.begin(), 0));
  EXPECT_TRUE(WaitUntil(yaclib_std::chrono::steady_clock::now(), fs.begin(), fs.end()));
}

TEST(Wait, JustWorks) {
  TestJustWorks<WaitPolicy::Endless>();
}

#if !defined(YACLIB_TSAN) || !defined(__GNUC__)  // https://github.com/google/sanitizers/issues/1259
TEST(WaitFor, JustWorks) {
  TestJustWorks<WaitPolicy::For>();
}
#endif

TEST(WaitUntil, JustWorks) {
  TestJustWorks<WaitPolicy::Until>();
}

template <WaitPolicy kPolicy>
void TestMultiThreaded() {
  static constexpr int kThreads = 4;
  yaclib::FairThreadPool tp{kThreads};
  yaclib::FutureOn<int> fs[kThreads];

  for (int i = 0; i < kThreads; ++i) {
    fs[i] = yaclib::Run(tp, [i] {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return i;
    });
  }
  test::util::StopWatch timer;
  if constexpr (kPolicy == WaitPolicy::Endless) {
    Wait(fs[0], fs[1], fs[2], fs[3]);
    EXPECT_LE(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
  } else if constexpr (kPolicy == WaitPolicy::For) {
    bool ready = WaitFor(100ms * YACLIB_CI_SLOWDOWN, fs[0], fs[1], fs[2], fs[3]);
    EXPECT_TRUE(ready);
  } else if constexpr (kPolicy == WaitPolicy::Until) {
    bool ready =
      WaitUntil(yaclib_std::chrono::system_clock::now() + 100ms * YACLIB_CI_SLOWDOWN, fs[0], fs[1], fs[2], fs[3]);
    EXPECT_TRUE(ready);
  }
  EXPECT_LE(timer.Elapsed(), 150ms * YACLIB_CI_SLOWDOWN);

  EXPECT_TRUE(std::all_of(std::begin(fs), std::end(fs), [](auto& f) {
    return f.Ready();
  }));

  for (int i = 0; i < kThreads; ++i) {
    EXPECT_EQ(std::move(fs[i]).Get().Value(), i);
  }
  tp.Stop();
  tp.Wait();
}

TEST(Wait, Multithreaded) {
  TestMultiThreaded<WaitPolicy::Endless>();
}

#if !defined(YACLIB_TSAN) || !defined(__GNUC__)  // https://github.com/google/sanitizers/issues/1259
TEST(WaitFor, Multithreaded) {
  TestMultiThreaded<WaitPolicy::For>();
}
#endif

TEST(WaitUntil, Multithreaded) {
  TestMultiThreaded<WaitPolicy::Until>();
}

template <WaitPolicy kPolicy>
void TestHaveResults() {
  auto [f1, p1] = yaclib::MakeContract<>();
  auto [f2, p2] = yaclib::MakeContract<>();
  yaclib::FairThreadPool tp{1};
  Submit(tp, [p1 = std::move(p1), p2 = std::move(p2)]() mutable {
    std::move(p1).Set();
    std::move(p2).Set();
  });
  yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
  if constexpr (kPolicy == WaitPolicy::Endless) {
    Wait(f1, f2);
  } else if constexpr (kPolicy == WaitPolicy::Until) {
    EXPECT_TRUE(WaitUntil(yaclib_std::chrono::steady_clock::now(), f1, f2));
  } else {
    EXPECT_TRUE(WaitFor(0ns, f1, f2));
  }

  EXPECT_TRUE(f1.Ready());
  EXPECT_TRUE(f2.Ready());

  tp.HardStop();
  tp.Wait();
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

TEST(WaitFor, Diff) {
  static constexpr int kThreads = 4;
  yaclib::FairThreadPool tp{kThreads};
  yaclib::FutureOn<int> fs[kThreads];

  for (int i = 0; i < kThreads; ++i) {
    fs[i] = yaclib::Run(tp, [i] {
      yaclib_std::this_thread::sleep_for(i * 50ms * YACLIB_CI_SLOWDOWN);
      return i;
    });
  }
  test::util::StopWatch timer;
  bool ready = WaitFor(100ms * YACLIB_CI_SLOWDOWN, std::begin(fs), std::end(fs));
  EXPECT_FALSE(ready);
  EXPECT_LE(timer.Elapsed(), 150ms * YACLIB_CI_SLOWDOWN);

  EXPECT_TRUE(std::any_of(std::begin(fs), std::end(fs), [](auto& f) {
    return f.Ready();
  }));

  EXPECT_FALSE(std::all_of(std::begin(fs), std::end(fs), [](auto& f) {
    return f.Ready();
  }));

  for (int i = 0; i < kThreads; ++i) {
    EXPECT_EQ(std::move(fs[i]).Get().Value(), i);
  }
  tp.Stop();
  tp.Wait();
}

TEST(Wait, ResetWait) {
  yaclib::FairThreadPool tp;
  std::vector<yaclib::FutureOn<std::size_t>> fs;
  fs.reserve(1000 * yaclib_std::thread::hardware_concurrency());
  for (std::size_t i = 0; i != 1000 * yaclib_std::thread::hardware_concurrency(); ++i) {
    fs.push_back(yaclib::Run(tp, [i] {
      test::util::Reschedule();
      return i;
    }));
  }
  yaclib::WaitFor(0ns, fs.begin(), fs.end());
  tp.HardStop();
  tp.Wait();
}
// silly test to pass codecov
TEST(SillyTest, ForTouch) {
  yaclib::FairThreadPool tp;
  yaclib::FutureOn<int> fut = yaclib::Run(tp, [] {
    yaclib_std::this_thread::sleep_for(1ms);
    return 42;
  });
  yaclib::Wait(fut);
  EXPECT_EQ(std::move(fut).Touch().Ok(), 42);

  fut = yaclib::Run(tp, [] {
    yaclib_std::this_thread::sleep_for(1ms);
    return 42;
  });

  yaclib::Wait(fut);
  auto res = std::as_const(fut).Touch();
  EXPECT_EQ(std::move(res).Ok(), 42);
  tp.HardStop();
  tp.Wait();
}

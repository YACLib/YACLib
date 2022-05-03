#include <util/time.hpp>

#include <yaclib/algo/wait.hpp>
#include <yaclib/coroutine/async_mutex.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/coroutine/on.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace test {
namespace {

void Stress1(const std::size_t kCoros, test::util::Duration dur) {
  auto tp = yaclib::MakeThreadPool();
  auto mutex = yaclib::AsyncMutex();
  std::vector<yaclib::Future<void>> futures(kCoros);
  std::size_t cs = 0;
  test::util::StopWatch sw;

  auto coro = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    while (sw.Elapsed() < dur) {
      auto guard = co_await mutex.Guard();
      if (std::numeric_limits<std::size_t>::max() != cs) {
        cs++;
      }
      if (cs == 7) {
        co_await On(*tp);
      }
    }
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    futures[i] = coro();
  }

  Wait(futures.begin(), futures.end());
  ASSERT_GE(cs, 1234);
  tp->HardStop();
  tp->Wait();
}

void Stress2(const std::size_t kCoros, test::util::Duration dur) {
  auto tp = yaclib::MakeThreadPool();
  auto mutex = yaclib::AsyncMutex();
  std::vector<yaclib::Future<void>> futures(kCoros);
  std::size_t cs;
  test::util::StopWatch sw;

  auto coro = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    if (std::numeric_limits<std::size_t>::max() != cs) {
      cs++;
    }
    co_await mutex.Unlock(*tp);
  };

  while (sw.Elapsed() < dur) {
    cs = 0;
    for (std::size_t i = 0; i < kCoros; ++i) {
      futures[i] = coro();
    }
    Wait(futures.begin(), futures.end());
    ASSERT_GE(cs, kCoros);
  }
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutexStress, TimerPerCoro) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
  using namespace std::chrono_literals;
  Stress1(4, 1s * YACLIB_CI_SLOWDOWN);
  Stress1(100, 1s * YACLIB_CI_SLOWDOWN);
}

TEST(AsyncMutexStress, CommonTimer) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
  using namespace std::chrono_literals;
  Stress2(4, 1s * YACLIB_CI_SLOWDOWN);
  Stress2(100, 1s * YACLIB_CI_SLOWDOWN);
}

}  // namespace
}  // namespace test

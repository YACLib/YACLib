#include <util/time.hpp>

#include <yaclib/async/wait.hpp>
#include <yaclib/coro/async_mutex.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace test {
namespace {

void Stress1(const std::size_t kCoros, test::util::Duration dur) {
  auto tp = yaclib::MakeThreadPool();
  yaclib::AsyncMutex<> m;
  std::vector<yaclib::Future<>> futures(kCoros);
  std::size_t cs = 0;
  test::util::StopWatch sw;

  auto coro = [&]() -> yaclib::Future<> {
    co_await On(*tp);
    while (sw.Elapsed() < dur) {
      auto guard = co_await m.Guard();
      if (std::numeric_limits<std::size_t>::max() != cs) {
        cs++;
      }
      if (cs == 7) {
        co_await On(*tp);
      }
    }
    co_return{};
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
  yaclib::AsyncMutex<> m;
  std::vector<yaclib::Future<>> futures(kCoros);
  std::uint64_t cs;
  test::util::StopWatch sw;

  auto coro = [&]() -> yaclib::Future<> {
    co_await On(*tp);
    co_await m.Lock();
    ++cs;
    co_await m.Unlock(*tp);
    co_return{};
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
  GTEST_SKIP();  // TODO(myannyax): make time run forward even without switches
#endif
  using namespace std::chrono_literals;
  Stress1(4, 1s);
  Stress1(100, 1s);
}

TEST(AsyncMutexStress, CommonTimer) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // TODO(myannyax): make time run forward even without switches
#endif
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  using namespace std::chrono_literals;
  Stress2(4, 1s);
  Stress2(100, 1s);
}

}  // namespace
}  // namespace test

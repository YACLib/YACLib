#include <util/time.hpp>

#include <yaclib/async/wait.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/coro/mutex.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace test {
namespace {

void Stress1(const std::size_t coros, test::util::Duration dur) {
  yaclib::FairThreadPool tp;
  yaclib::Mutex<> m;
  std::vector<yaclib::Future<>> futures(coros);
  std::uint64_t cs = 0;
  test::util::StopWatch sw;

  auto coro = [&]() -> yaclib::Future<> {
    co_await On(tp);
    while (sw.Elapsed() < dur) {
      auto g = co_await m.Guard();
      ++cs;
      if (cs == 7) {
        co_await On(tp);
      }
    }
    co_return{};
  };

  for (std::size_t i = 0; i < coros; ++i) {
    futures[i] = coro();
  }

  Wait(futures.begin(), futures.end());
  ASSERT_GE(cs, 500);
  tp.HardStop();
  tp.Wait();
}

void Stress2(const std::size_t coros, test::util::Duration dur) {
  yaclib::FairThreadPool tp;
  yaclib::Mutex<> m;
  std::vector<yaclib::Future<>> futures(coros);
  std::uint64_t cs = 0;
  test::util::StopWatch sw;

  auto coro = [&]() -> yaclib::Future<> {
    co_await On(tp);
    co_await m.Lock();
    ++cs;
    co_await m.Unlock();
    co_return{};
  };

  while (sw.Elapsed() < dur) {
    cs = 0;
    for (std::size_t i = 0; i < coros; ++i) {
      futures[i] = coro();
    }
    Wait(futures.begin(), futures.end());
    ASSERT_GE(cs, coros);
  }
  tp.HardStop();
  tp.Wait();
}

TEST(MutexStress, TimerPerCoro) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // TODO(myannyax) make time run forward even without switches
#endif
  using namespace std::chrono_literals;
  Stress1(4, 1s);
  Stress1(100, 1s);
}

TEST(MutexStress, CommonTimer) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // TODO(myannyax) make time run forward even without switches
#endif
#if defined(GTEST_OS_WINDOWS) && !(defined(NDEBUG) && defined(_WIN64))
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  using namespace std::chrono_literals;
  Stress2(4, 1s);
  Stress2(100, 1s);
}

}  // namespace
}  // namespace test

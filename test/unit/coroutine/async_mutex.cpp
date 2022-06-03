#include <util/intrusive_list.hpp>
#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/async_mutex.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/coroutine/on.hpp>
#include <yaclib/executor/manual.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <array>
#include <exception>
#include <mutex>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(AsyncMutex, JustWorks) {
  yaclib::AsyncMutex m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 10'000;

  std::array<yaclib::Future<void>, kCoros> futures;
  std::size_t cs = 0;

  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await m.Lock();
    ++cs;
    m.UnlockHere();
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    futures[i] = coro1();
  }

  yaclib::Wait(futures.begin(), futures.size());

  EXPECT_EQ(kCoros, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, Counter) {
  yaclib::AsyncMutex<true> m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 2000;
  yaclib::WaitGroup wg;
  std::array<yaclib::Future<void>, kCoros> futures;
  std::size_t cs = 0;

  auto coro1 = [&]() -> yaclib::Future<void> {
    for (std::size_t j = 0; j < kCSperCoro; ++j) {
      co_await On(*tp);
      co_await m.Lock();
      ++cs;
      co_await m.Unlock();
    }
  };

  wg.Add(kCoros);
  for (std::size_t i = 0; i < kCoros; ++i) {
    futures[i] = coro1();
    wg.Add<false>(futures[i]);
  }
  wg.Wait();

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, TryLock) {
  yaclib::AsyncMutex mutex;
  EXPECT_TRUE(mutex.TryLock());
  EXPECT_FALSE(mutex.TryLock());
  mutex.UnlockHere();
  EXPECT_TRUE(mutex.TryLock());
  mutex.UnlockHere();
}

TEST(AsyncMutex, ScopedLock) {
  yaclib::AsyncMutex m;
  {
    auto lock = m.TryGuard();
    EXPECT_TRUE(lock.OwnsLock());
    {
      auto lock2 = m.TryGuard();
      EXPECT_FALSE(lock2.OwnsLock());
    }
  }
  EXPECT_TRUE(m.TryLock());
  m.UnlockHere();
}

TEST(AsyncMutex, LockAsync) {
  yaclib::AsyncMutex m;
  auto executor = yaclib::MakeManual();
  auto tp = yaclib::MakeThreadPool();
  auto [f1, p1] = yaclib::MakeContract<bool>();
  auto [f2, p2] = yaclib::MakeContract<bool>();

  std::size_t value = 0;

  auto coro = [&](yaclib::Future<bool>& future) -> yaclib::Future<void> {
    co_await On(*executor);
    co_await m.Lock();
    value++;
    co_await Await(future);
    value++;
    co_await m.UnlockOn(*tp);
  };

  auto c1 = coro(f1);
  executor->Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  auto c2 = coro(f2);
  executor->Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p1).Set(true);
  executor->Drain();
  EXPECT_EQ(3, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p2).Set(true);
  executor->Drain();

  EXPECT_EQ(4, value);
  EXPECT_TRUE(m.TryLock());
  m.UnlockHere();
}

TEST(AsyncMutex, ScopedLockAsync) {
  yaclib::AsyncMutex m;
  auto executor = yaclib::MakeManual();
  auto tp = yaclib::MakeThreadPool();
  auto [f1, p1] = yaclib::MakeContract<bool>();
  auto [f2, p2] = yaclib::MakeContract<bool>();

  std::size_t value = 0;

  auto coro = [&](yaclib::Future<bool>& future) -> yaclib::Future<void> {
    co_await On(*executor);
    auto g = co_await m.Guard();
    value++;
    co_await Await(future);
    value++;
  };

  auto c1 = coro(f1);
  executor->Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  auto c2 = coro(f2);
  executor->Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p1).Set(true);
  executor->Drain();
  EXPECT_EQ(3, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p2).Set(true);
  executor->Drain();

  EXPECT_EQ(4, value);
  EXPECT_TRUE(m.TryLock());
  m.UnlockHere();
}

TEST(AsyncMutex, GuardRelease) {
  yaclib::AsyncMutex m;
  auto tp = yaclib::MakeThreadPool(2);

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 200;

  std::array<yaclib::Future<int>, kCoros> futures;
  yaclib::WaitGroup wg;
  std::size_t cs = 0;

  auto coro1 = [&]() -> yaclib::Future<int> {
    for (std::size_t j = 0; j < kCSperCoro; ++j) {
      co_await On(*tp);
      auto g = co_await m.Guard();
      auto another = yaclib::AsyncMutex<>::LockGuard(*g.Release(), std::adopt_lock_t{});
      ++cs;
      co_await another.UnlockOn(*tp);
    }
    co_return 42;
  };
  wg.Add(kCoros);
  for (std::size_t i = 0; i < kCoros; ++i) {
    futures[i] = coro1();
    wg.Add<false>(futures[i]);
  }

  wg.Wait();

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, UnlockHereBehaviour) {
  using namespace std::chrono_literals;
  yaclib::AsyncMutex mutex;
  constexpr std::size_t kThreads = 4;
  auto tp = yaclib::MakeThreadPool(kThreads);
  auto& inln = yaclib::MakeInline();
  constexpr std::size_t kCoros = 4;

  std::array<yaclib::Future<void>, kCoros> futures;

  util::StopWatch sw;

  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    yaclib_std::this_thread::sleep_for(1s * YACLIB_CI_SLOWDOWN);
    mutex.UnlockHere();
  };

  futures[0] = coro1();

  yaclib_std::this_thread::sleep_for(128ms * YACLIB_CI_SLOWDOWN);

  auto coro2 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    mutex.UnlockHere();
    yaclib_std::this_thread::sleep_for(1s * YACLIB_CI_SLOWDOWN);
  };

  for (std::size_t i = 1; i < kCoros; ++i) {
    futures[i] = coro2();
  }

  Wait(futures.begin(), futures.end());

  EXPECT_LE(sw.Elapsed(), 2.5s * YACLIB_CI_SLOWDOWN);

  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, UnlockOnBehaviour) {
  using namespace std::chrono_literals;
  yaclib::AsyncMutex mutex;
  constexpr std::size_t kThreads = 4;
  auto tp = yaclib::MakeThreadPool(kThreads);
  auto& inln = yaclib::MakeInline();
  constexpr std::size_t kCoros = 4;

  auto tp2 = yaclib::MakeThreadPool(1);

  std::array<yaclib::Future<void>, kCoros> futures;

  util::StopWatch sw;
  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    yaclib_std::this_thread::sleep_for(1s * YACLIB_CI_SLOWDOWN);
    co_await mutex.UnlockOn(*tp2);
  };

  futures[0] = coro1();

  yaclib_std::this_thread::sleep_for(128ms * YACLIB_CI_SLOWDOWN);

  auto coro2 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    co_await mutex.UnlockOn(*tp2);
    yaclib_std::this_thread::sleep_for(0.5s * YACLIB_CI_SLOWDOWN);
  };

  for (std::size_t i = 1; i < kCoros; ++i) {
    futures[i] = coro2();
  }

  Wait(futures.begin(), futures.end());

  EXPECT_GE(sw.Elapsed(), 2.5s * YACLIB_CI_SLOWDOWN);
  tp2->HardStop();
  tp2->Wait();
  tp->HardStop();
  tp->Wait();
}

}  // namespace
}  // namespace test

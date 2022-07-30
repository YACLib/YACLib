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
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  yaclib::AsyncMutex<true> m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 2000;
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

  for (std::size_t i = 0; i < kCoros; ++i) {
    futures[i] = coro1();
  }
  Wait(futures.begin(), futures.end());

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
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
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
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  yaclib::AsyncMutex m;
  auto tp = yaclib::MakeThreadPool(2);

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 200;

  std::array<yaclib::Future<int>, kCoros> futures;
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
  for (std::size_t i = 0; i < kCoros; ++i) {
    futures[i] = coro1();
  }

  Wait(futures.begin(), futures.end());

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, UnlockHereBehaviour) {
  using namespace std::chrono_literals;
  constexpr std::size_t kThreads = 4;
  constexpr std::size_t kCoros = 4;

  auto tp = yaclib::MakeThreadPool(kThreads);
  yaclib::AsyncMutex mutex;
  std::array<yaclib::Future<void>, kCoros> futures;
  yaclib_std::atomic_bool start{false};

  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    start.store(true, std::memory_order_release);
    auto id = yaclib_std::this_thread::get_id();
    yaclib_std::this_thread::sleep_for(1s);
    mutex.UnlockHere();
    EXPECT_EQ(id, yaclib_std::this_thread::get_id());
  };
  auto coro2 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    auto id = yaclib_std::this_thread::get_id();
    mutex.UnlockHere();
    yaclib_std::this_thread::sleep_for(1s);
    EXPECT_EQ(id, yaclib_std::this_thread::get_id());
  };

  util::StopWatch sw;
  futures[0] = coro1();
  while (!start.load(std::memory_order_acquire)) {
    yaclib_std::this_thread::sleep_for(10ms);
  }
  for (std::size_t i = 1; i < kCoros; ++i) {
    futures[i] = coro2();
  }
  Wait(futures.begin(), futures.end());

  // 1s (coro1 sleep with acquired lock) + 1s (coro2 parallel sleep)
  EXPECT_LT(sw.Elapsed(), 2.5s);

  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, UnlockOnBehaviour) {
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  using namespace std::chrono_literals;
  constexpr std::size_t kThreads = 4;
  constexpr std::size_t kCoros = 4;

  auto tp = yaclib::MakeThreadPool(kThreads);
  yaclib::AsyncMutex mutex;
  std::array<yaclib::Future<void>, kCoros> futures;

  yaclib_std::atomic_bool start{false};
  yaclib_std::thread::id locked_id{};

  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
    start.store(true, std::memory_order_release);
    locked_id = yaclib_std::this_thread::get_id();
    yaclib_std::this_thread::sleep_for(1s);
    co_await mutex.UnlockOn(*tp);
  };
  auto coro2 = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    co_await mutex.Lock();
#ifdef GTEST_OS_LINUX
    EXPECT_EQ(locked_id, yaclib_std::this_thread::get_id());
#endif
    co_await mutex.UnlockOn(*tp);
    yaclib_std::this_thread::sleep_for(1s);
  };

  util::StopWatch sw;
  futures[0] = coro1();
  while (!start.load(std::memory_order_acquire)) {
    yaclib_std::this_thread::sleep_for(10ms);
  }
  for (std::size_t i = 1; i < kCoros; ++i) {
    futures[i] = coro2();
  }
  Wait(futures.begin(), futures.end());

  // 1s (coro1 sleep with acquired lock) + 1s (coro2 parallel sleep)
  EXPECT_LT(sw.Elapsed(), 2.5s);

  tp->HardStop();
  tp->Wait();
}

}  // namespace
}  // namespace test

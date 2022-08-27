#include <util/async_suite.hpp>
#include <util/intrusive_list.hpp>
#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coro/async_mutex.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/coro/task.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <array>
#include <exception>
#include <mutex>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

TYPED_TEST(AsyncSuite, JustWorks) {
  yaclib::AsyncMutex<> m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 10'000;

  std::array<yaclib::Future<>, kCoros> futures;
  std::size_t cs = 0;

  auto coro1 = [&]() -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*tp);
    }
    co_await m.Lock();
    ++cs;
    m.UnlockHere();
    co_return{};
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    if constexpr (TestFixture::kIsFuture) {
      futures[i] = coro1();
    } else {
      futures[i] = coro1().ToFuture(*tp).On(nullptr);
    }
  }

  yaclib::Wait(futures.begin(), futures.size());

  EXPECT_EQ(kCoros, cs);
  tp->HardStop();
  tp->Wait();
}

TYPED_TEST(AsyncSuite, Counter) {
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  yaclib::AsyncMutex<true> m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 2000;
  std::array<yaclib::Future<>, kCoros> futures;
  std::size_t cs = 0;

  auto coro1 = [&]() -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*tp);
    }
    for (std::size_t j = 0; j < kCSperCoro; ++j) {
      co_await m.Lock();
      ++cs;
      co_await m.Unlock();
    }
    co_return{};
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    if constexpr (TestFixture::kIsFuture) {
      futures[i] = coro1();
    } else {
      futures[i] = coro1().ToFuture(*tp).On(nullptr);
    }
  }
  Wait(futures.begin(), futures.end());

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutexSuite, TryLock) {
  yaclib::AsyncMutex<> m;
  EXPECT_TRUE(m.TryLock());
  EXPECT_FALSE(m.TryLock());
  m.UnlockHere();
  EXPECT_TRUE(m.TryLock());
  m.UnlockHere();
}

TEST(AsyncMutexSuite, ScopedLock) {
  yaclib::AsyncMutex<> m;
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

TYPED_TEST(AsyncSuite, LockAsync) {
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  yaclib::AsyncMutex<> m;
  auto executor = yaclib::MakeManual();
  auto [f1, p1] = yaclib::MakeContract<bool>();
  auto [f2, p2] = yaclib::MakeContract<bool>();

  std::size_t value = 0;

  auto coro = [&](yaclib::Future<bool>& future) -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*executor);
    }
    co_await m.Lock();
    value++;
    co_await Await(future);
    value++;
    co_await m.UnlockOn(*executor);
    co_return{};
  };
  auto run_coro = [&](auto&& func, auto&& arg) {
    if constexpr (TestFixture::kIsFuture) {
      return func(arg);
    } else {
      return func(arg).ToFuture(*executor).On(nullptr);
    }
  };
  auto c1 = run_coro(coro, f1);
  executor->Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  auto c2 = run_coro(coro, f2);
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

TYPED_TEST(AsyncSuite, ScopedLockAsync) {
  yaclib::AsyncMutex<> m;
  auto executor = yaclib::MakeManual();
  auto [f1, p1] = yaclib::MakeContract<bool>();
  auto [f2, p2] = yaclib::MakeContract<bool>();

  std::size_t value = 0;

  auto coro = [&](yaclib::Future<bool>& future) -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*executor);
    }
    auto g = co_await m.Guard();
    value++;
    co_await Await(future);
    value++;
    co_return{};
  };

  auto run_coro = [&](auto&& func, auto&& arg) {
    if constexpr (TestFixture::kIsFuture) {
      return func(arg);
    } else {
      return func(arg).ToFuture(*executor).On(nullptr);
    }
  };

  auto c1 = run_coro(coro, f1);
  executor->Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  auto c2 = run_coro(coro, f2);
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

TYPED_TEST(AsyncSuite, GuardRelease) {
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  yaclib::AsyncMutex<> m;
  auto tp = yaclib::MakeThreadPool(2);

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 200;

  std::array<yaclib::Future<int>, kCoros> futures;
  std::size_t cs = 0;
  using Coro = std::conditional_t<TestFixture::kIsFuture, yaclib::Future<int>, yaclib::Task<int>>;
  auto coro1 = [&]() -> Coro {
    for (std::size_t j = 0; j < kCSperCoro; ++j) {
      if constexpr (TestFixture::kIsFuture) {
        co_await On(*tp);
      }
      auto g = co_await m.Guard();
      auto another = yaclib::AsyncMutex<>::LockGuard(*g.Release(), std::adopt_lock_t{});
      ++cs;
      co_await another.UnlockOn(*tp);
    }
    co_return 42;
  };
  for (std::size_t i = 0; i < kCoros; ++i) {
    if constexpr (TestFixture::kIsFuture) {
      futures[i] = coro1();
    } else {
      futures[i] = coro1().ToFuture(*tp).On(nullptr);
    }
  }

  Wait(futures.begin(), futures.end());

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

TYPED_TEST(AsyncSuite, UnlockHereBehaviour) {
  using namespace std::chrono_literals;
  constexpr std::size_t kThreads = 4;
  constexpr std::size_t kCoros = 4;

  auto tp = yaclib::MakeThreadPool(kThreads);
  yaclib::AsyncMutex<> m;
  std::array<yaclib::Future<>, kCoros> futures;
  yaclib_std::atomic_bool start{false};

  auto coro1 = [&]() -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*tp);
    }
    co_await m.Lock();
    start.store(true, std::memory_order_release);
    auto id = yaclib_std::this_thread::get_id();
    yaclib_std::this_thread::sleep_for(1s);
    m.UnlockHere();
    EXPECT_EQ(id, yaclib_std::this_thread::get_id());
    co_return{};
  };
  auto coro2 = [&]() -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*tp);
    }
    co_await m.Lock();
    auto id = yaclib_std::this_thread::get_id();
    m.UnlockHere();
    yaclib_std::this_thread::sleep_for(1s);
    EXPECT_EQ(id, yaclib_std::this_thread::get_id());
    co_return{};
  };

  util::StopWatch sw;
  if constexpr (TestFixture::kIsFuture) {
    futures[0] = coro1();
  } else {
    futures[0] = coro1().ToFuture(*tp).On(nullptr);
  }
  while (!start.load(std::memory_order_acquire)) {
    yaclib_std::this_thread::sleep_for(10ms);
  }
  for (std::size_t i = 1; i < kCoros; ++i) {
    if constexpr (TestFixture::kIsFuture) {
      futures[i] = coro2();
    } else {
      futures[i] = coro2().ToFuture(*tp).On(nullptr);
    }
  }
  Wait(futures.begin(), futures.end());

  // 1s (coro1 sleep with acquired lock) + 1s (coro2 parallel sleep)
  EXPECT_LT(sw.Elapsed(), 2.5s);

  tp->HardStop();
  tp->Wait();
}

TYPED_TEST(AsyncSuite, UnlockOnBehaviour) {
#ifdef GTEST_OS_WINDOWS
  GTEST_SKIP();  // Doesn't work for Win32 or Debug, I think its probably because bad symmetric transfer implementation
  // TODO(kononovk) Try to confirm problem and localize it with ifdefs
#endif
  using namespace std::chrono_literals;
  constexpr std::size_t kThreads = 4;
  constexpr std::size_t kCoros = 4;

  auto tp = yaclib::MakeThreadPool(kThreads);
  yaclib::AsyncMutex<> m;
  std::array<yaclib::Future<>, kCoros> futures;

  yaclib_std::atomic_bool start{false};
  yaclib_std::thread::id locked_id{};

  auto coro1 = [&]() -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*tp);
    }
    co_await m.Lock();
    start.store(true, std::memory_order_release);
    locked_id = yaclib_std::this_thread::get_id();
    yaclib_std::this_thread::sleep_for(1s);
    co_await m.UnlockOn(*tp);
    co_return{};
  };
  auto coro2 = [&]() -> typename TestFixture::Type {
    if constexpr (TestFixture::kIsFuture) {
      co_await On(*tp);
    }
    co_await m.Lock();
#ifdef GTEST_OS_LINUX
    EXPECT_EQ(locked_id, yaclib_std::this_thread::get_id());
#endif
    co_await m.UnlockOn(*tp);
    yaclib_std::this_thread::sleep_for(1s);
    co_return{};
  };

  util::StopWatch sw;
  if constexpr (TestFixture::kIsFuture) {
    futures[0] = coro1();
  } else {
    futures[0] = coro1().ToFuture(*tp).On(nullptr);
  }
  while (!start.load(std::memory_order_acquire)) {
    yaclib_std::this_thread::sleep_for(10ms);
  }
  for (std::size_t i = 1; i < kCoros; ++i) {
    if constexpr (TestFixture::kIsFuture) {
      futures[i] = coro2();
    } else {
      futures[i] = coro2().ToFuture(*tp).On(nullptr);
    }
  }
  Wait(futures.begin(), futures.end());

  // 1s (coro1 sleep with acquired lock) + 1s (coro2 parallel sleep)
  EXPECT_LT(sw.Elapsed(), 2.5s);

  tp->HardStop();
  tp->Wait();
}

}  // namespace
}  // namespace test

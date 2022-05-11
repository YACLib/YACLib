#include <util/intrusive_list.hpp>
#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/async_mutex.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/coroutine/on.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/detail/nope_counter.hpp>

#include <exception>
#include <utility>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace {

TEST(AsyncMutex, JustWorks) {
  yaclib::AsyncMutex m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 10'000;

  std::array<yaclib::Future<void>, kCoros> futures;
  yaclib::WaitGroup wg;
  std::size_t cs = 0;

  yaclib_std::atomic<int> done = 0;

  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await m.Lock();
    cs++;
    m.UnlockHere();
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    yaclib::Submit(*tp, [&, i]() {
      wg.Add(futures[i] = coro1());
      done.fetch_add(1, std::memory_order_seq_cst);
    });
  }

  while (!(done.load(std::memory_order_seq_cst) == kCoros)) {
  }
  wg.Wait();

  EXPECT_EQ(kCoros, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, Counter) {
  yaclib::AsyncMutex<true> m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 2000;

  std::array<yaclib::Future<void>, kCoros> futures;
  yaclib::WaitGroup wg;
  std::size_t cs = 0;

  yaclib_std::atomic<int> done = 0;

  auto coro1 = [&]() -> yaclib::Future<void> {
    for (std::size_t j = 0; j < kCSperCoro; ++j) {
      co_await m.Lock();
      cs++;
      co_await m.Unlock();
    }
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    yaclib::Submit(*tp, [&, i]() {
      wg.Add(futures[i] = coro1());
      done.fetch_add(1, std::memory_order_seq_cst);
    });
  }

  while (!(done.load(std::memory_order_seq_cst) == kCoros)) {
  }
  wg.Wait();

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

TEST(AsyncMutex, Blocking) {
  using namespace std::chrono_literals;

  yaclib::AsyncMutex m;
  auto tp = yaclib::MakeThreadPool();
  test::util::StopWatch<> timer;
  yaclib::WaitGroup<> wg;
  yaclib::Future<void> f1, f2;

  yaclib::Submit(*tp, [&]() {
    auto coro = [&]() -> yaclib::Future<void> {
      co_await m.Lock();
      yaclib_std::this_thread::sleep_for(1s * YACLIB_CI_SLOWDOWN);
      co_await m.Unlock(*tp);
    };
    f1 = coro();
    wg.Add(f1);
  });

  yaclib::Submit(*tp, [&]() {
    auto coro = [&]() -> yaclib::Future<void> {
      co_await m.Lock();
      co_await m.Unlock(*tp);
    };
    f2 = coro();
    wg.Add(f2);
  });

  wg.Wait();

  EXPECT_LT(timer.Elapsed(), 100ms * YACLIB_CI_SLOWDOWN);
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
  class ManualExecutor : public yaclib::IExecutor {
   private:
    yaclib::detail::List _tasks;

   public:
    [[nodiscard]] Type Tag() const final {
      return yaclib::IExecutor::Type::Custom;
    }

    void Submit(yaclib::Job& f) noexcept final {
      _tasks.PushFront(f);
    }

    void Drain() {
      while (!_tasks.Empty()) {
        auto& task = _tasks.PopFront();
        static_cast<yaclib::Job&>(task).Call();
      }
    }

    ~ManualExecutor() override {
      EXPECT_TRUE(_tasks.Empty());
    }
  };
  yaclib::AsyncMutex m;
  yaclib::detail::NopeCounter<ManualExecutor> executor;
  auto tp = yaclib::MakeThreadPool();
  auto [f1, p1] = yaclib::MakeContract<bool>();
  auto [f2, p2] = yaclib::MakeContract<bool>();

  std::size_t value = 0;

  auto coro = [&](yaclib::Future<bool>& future) -> yaclib::Future<void> {
    co_await On(executor);
    co_await m.Lock();
    value++;
    co_await Await(future);
    value++;
    co_await m.Unlock<yaclib::AsyncMutex<>::UnlockType::On>(*tp);
  };

  auto c1 = coro(f1);
  executor.Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  auto c2 = coro(f2);
  executor.Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p1).Set(true);
  executor.Drain();
  EXPECT_EQ(3, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p2).Set(true);
  executor.Drain();

  EXPECT_EQ(4, value);
  EXPECT_TRUE(m.TryLock());
  m.UnlockHere();
}

TEST(AsyncMutex, ScopedLockAsync) {
  class ManualExecutor : public yaclib::IExecutor {
   private:
    yaclib::detail::List _tasks;

   public:
    [[nodiscard]] Type Tag() const final {
      return yaclib::IExecutor::Type::Custom;
    }

    void Submit(yaclib::Job& f) noexcept final {
      _tasks.PushFront(f);
    }

    void Drain() {
      while (!_tasks.Empty()) {
        auto& task = _tasks.PopFront();
        static_cast<yaclib::Job&>(task).Call();
      }
    }

    ~ManualExecutor() override {
      EXPECT_TRUE(_tasks.Empty());
    }
  };
  yaclib::AsyncMutex m;
  yaclib::detail::NopeCounter<ManualExecutor> executor;
  auto tp = yaclib::MakeThreadPool();
  auto [f1, p1] = yaclib::MakeContract<bool>();
  auto [f2, p2] = yaclib::MakeContract<bool>();

  std::size_t value = 0;

  auto coro = [&](yaclib::Future<bool>& future) -> yaclib::Future<void> {
    co_await On(executor);
    auto g = co_await m.Guard();
    value++;
    co_await Await(future);
    value++;
  };

  auto c1 = coro(f1);
  executor.Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  auto c2 = coro(f2);
  executor.Drain();
  EXPECT_EQ(1, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p1).Set(true);
  executor.Drain();
  EXPECT_EQ(3, value);
  EXPECT_FALSE(m.TryLock());

  std::move(p2).Set(true);
  executor.Drain();

  EXPECT_EQ(4, value);
  EXPECT_TRUE(m.TryLock());
  m.UnlockHere();
}

TEST(AsyncMutex, GuardRelease) {
  yaclib::AsyncMutex<> m;
  auto tp = yaclib::MakeThreadPool();

  const std::size_t kCoros = 20;
  const std::size_t kCSperCoro = 2000;

  std::array<yaclib::Future<void>, kCoros> futures;
  yaclib::WaitGroup wg;
  std::size_t cs = 0;

  yaclib_std::atomic<int> done = 0;

  auto coro1 = [&]() -> yaclib::Future<void> {
    for (std::size_t j = 0; j < kCSperCoro; ++j) {
      auto g = co_await m.Guard();
      auto а = yaclib::AsyncMutex<>::LockGuard(*g.Release(), std::adopt_lock_t{});
      cs++;
    }
  };

  for (std::size_t i = 0; i < kCoros; ++i) {
    yaclib::Submit(*tp, [&, i]() {
      wg.Add(futures[i] = coro1());
      done.fetch_add(1, std::memory_order_seq_cst);
    });
  }

  while (!(done.load(std::memory_order_seq_cst) == kCoros)) {
  }
  wg.Wait();

  EXPECT_EQ(kCoros * kCSperCoro, cs);
  tp->HardStop();
  tp->Wait();
}

}  // namespace

#include <util/async_suite.hpp>
#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/current_executor.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/coro/guard_sticky.hpp>
#include <yaclib/coro/mutex.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/coro/shared_mutex.hpp>
#include <yaclib/coro/task.hpp>
#include <yaclib/coro/yield.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>

#include <array>
#include <exception>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

yaclib::Task<> Simple() {
  yaclib::SharedMutex<> m;
  {
    auto guard = co_await m.Guard();
    EXPECT_FALSE(m.TryLock());
    EXPECT_FALSE(m.TryLockShared());
  }
  {
    auto guard1 = m.TryGuard();
    EXPECT_TRUE(guard1);
  }
  {
    auto guard1 = m.TryGuardShared();
    EXPECT_TRUE(guard1);
    auto guard2 = m.TryGuardShared();
    EXPECT_TRUE(guard2);
    EXPECT_FALSE(m.TryLock());
  }
  co_return{};
}

TEST(SharedMutex, Simple) {
  std::ignore = Simple().Get();
}

yaclib::Task<int> ParallelReader(yaclib::SharedMutex<>& m, yaclib_std::atomic_size_t& counter, volatile const int& x) {
  auto guard = co_await m.GuardShared();
  counter.fetch_add(2, std::memory_order_relaxed);
  while (counter.load(std::memory_order_relaxed) != 1) {
    co_await yaclib::kYield;
  }
  int y = x;
  co_return y;
}

yaclib::Task<> Writer(yaclib::SharedMutex<>& m, volatile int& x) {
  auto guard = co_await m.Guard();
  x = 1;
  co_return{};
}

void TestParallelReaders(std::size_t num_readers, std::size_t threads) {
  yaclib::FairThreadPool tp{threads};
  yaclib::WaitGroup<> wg{1};
  yaclib::SharedMutex<> m;
  yaclib_std::atomic_size_t counter = 0;
  volatile int x = 2;
  auto writer = Writer(m, x);
  for (std::size_t i = 0; i != num_readers; ++i) {
    wg.Consume(ParallelReader(m, counter, x).ToFuture(tp));
  }
  while (counter.load(std::memory_order_relaxed) != 2 * num_readers) {
  }
  counter.store(1, std::memory_order_relaxed);
  std::ignore = std::move(writer).Get();

  wg.Done();
  wg.Wait();
  tp.Stop();
  tp.Wait();
}

TEST(SharedMutex, TestParallelReaders) {
  // https://github.com/golang/go/blob/master/src/sync/rwmutex_test.go
  TestParallelReaders(1, 4);
  TestParallelReaders(3, 4);
  TestParallelReaders(4, 2);
}

yaclib::Task<> Reader(yaclib::SharedMutex<>& rmw, std::size_t num_iterations, std::int32_t& activity, size_t index) {
  for (std::size_t i = 0; i != num_iterations; ++i) {
    auto guard = co_await rmw.GuardShared();
    yaclib::InjectFault();
    for (std::size_t j = 0; j != 100; j += 1) {
      EXPECT_EQ(activity, 0);
    }
    yaclib::InjectFault();
    guard.UnlockHere();
  }
  co_return{};
}

yaclib::Task<> Writer(yaclib::SharedMutex<>& rmw, std::size_t num_iterations, std::int32_t& activity, size_t index) {
  for (std::size_t i = 0; i != num_iterations; ++i) {
    auto guard = co_await rmw.Guard();
    EXPECT_EQ(activity, 0);
    activity += 10000;
    yaclib::InjectFault();
    for (std::size_t j = 0; j != 100; j += 1) {
      EXPECT_EQ(activity, 10000);
    }
    yaclib::InjectFault();
    activity -= 10000;
    EXPECT_EQ(activity, 0);
    guard.UnlockHere();
  }
  co_return{};
}

void HammerRWMutex(std::size_t threads, std::size_t num_readers, std::size_t num_iterations) {
  yaclib::FairThreadPool tp{threads};
  yaclib::WaitGroup<> wg{1};
  std::int32_t activity = 0;
  yaclib::SharedMutex<> rmw;
  // Number of active readers + 10000 * number of active writers.
  wg.Consume(Writer(rmw, num_iterations, activity, 0).ToFuture(tp));
  std::size_t i = 0;
  for (; i != num_readers / 2; ++i) {
    wg.Consume(Reader(rmw, num_iterations, activity, i).ToFuture(tp));
  }
  wg.Consume(Writer(rmw, num_iterations, activity, 1).ToFuture(tp));
  for (; i != num_readers; ++i) {
    wg.Consume(Reader(rmw, num_iterations, activity, i).ToFuture(tp));
  }

  wg.Done();
  wg.Wait();
  tp.Stop();
  tp.Wait();
}

void RwMutexTest(std::size_t n) {
  // https://github.com/golang/go/blob/master/src/sync/rwmutex_test.go
  HammerRWMutex(1, 1, n);
  HammerRWMutex(1, 3, n);
  HammerRWMutex(1, 10, n);
  HammerRWMutex(4, 1, n);
  HammerRWMutex(4, 3, n);
  HammerRWMutex(4, 10, n);
  HammerRWMutex(10, 1, n);
  HammerRWMutex(10, 3, n);
  HammerRWMutex(10, 10, n);
  HammerRWMutex(10, 5, n);
}

TEST(SharedMutex, RWMutexSmall) {
  RwMutexTest(5);
}

TEST(SharedMutex, RWMutexLarge) {
  RwMutexTest(1000);
}

}  // namespace
}  // namespace test

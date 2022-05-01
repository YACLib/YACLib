#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <array>
#include <exception>
#include <stack>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(Await, JustWorksPack) {
#if defined(YACLIB_UBSAN) && (defined(__GLIBCPP__) || defined(__GLIBCXX__))
  GTEST_SKIP();
#endif
  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<int> {
    auto f1 = yaclib::Run(*tp, [] {
      yaclib_std::this_thread::sleep_for(1ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    auto f2 = yaclib::Run(*tp, [] {
      return 2;
    });

    co_await Await(f1, f2);
    co_return std::move(f1).Touch().Ok() + std::move(f2).Touch().Ok();
  };
  auto f = coro(tp);
  EXPECT_EQ(std::move(f).Get().Ok(), 3);
  tp->HardStop();
  tp->Wait();
}

TEST(Await, JustWorksRange) {
#if defined(YACLIB_UBSAN) && (defined(__GLIBCPP__) || defined(__GLIBCXX__))
  GTEST_SKIP();
#endif
  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<int> {
    std::array<yaclib::FutureOn<int>, 2> arr;
    arr[0] = yaclib::Run(*tp, [] {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    arr[1] = yaclib::Run(*tp, [] {
      return 2;
    });

    co_await yaclib::Await(arr.begin(), 2);
    // co_return yaclib::StopTag{};
    co_return std::move(arr[0]).Touch().Ok() + std::move(arr[1]).Touch().Ok();
  };
  auto future = coro(tp);
  EXPECT_EQ(std::move(future).Get().Ok(), 3);
  tp->HardStop();
  tp->Wait();
}

TEST(Await, CheckSuspend) {
#if defined(YACLIB_UBSAN) && (defined(__GLIBCPP__) || defined(__GLIBCXX__))
  GTEST_SKIP();
#endif
  int counter = 0;
  auto tp = yaclib::MakeThreadPool(2);
  const auto coro_sleep_time = 50ms * YACLIB_CI_SLOWDOWN;
  auto was = std::chrono::steady_clock::now();
  std::atomic_bool barrier = false;

  auto coro = [&]() -> yaclib::Future<void> {
    counter = 1;
    auto future1 = yaclib::Run(*tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });
    auto future2 = yaclib::Run(*tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });

    co_await Await(future1, future2);

    EXPECT_TRUE(barrier.load(std::memory_order_acquire));
    while (!barrier.load(std::memory_order_acquire)) {
    }

    counter = 2;
    co_return;
  };

  auto outer_future = coro();

  EXPECT_EQ(1, counter);
  barrier.store(true, std::memory_order_release);

  EXPECT_LT(std::chrono::steady_clock::now() - was, coro_sleep_time);

  std::ignore = std::move(outer_future).Get();

  EXPECT_EQ(2, counter);
  EXPECT_GE(std::chrono::steady_clock::now() - was, coro_sleep_time);
  tp->HardStop();
  tp->Wait();
}

TEST(Await, NoSuspend) {
  int counter = 0;

  auto coro = [&]() -> yaclib::Future<void> {
    counter = 1;
    auto future = yaclib::MakeFuture();

    co_await Await(future);
    counter = 2;
    co_return;
  };

  auto outer_future = coro();
  EXPECT_EQ(2, counter);
  std::ignore = std::move(outer_future).Get();
}

}  // namespace

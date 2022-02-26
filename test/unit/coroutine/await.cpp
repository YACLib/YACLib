#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <array>
#include <exception>
#include <stack>
#include <utility>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(Await, JustWorksPack) {
  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<int> {
    auto f1 = yaclib::Run(tp, [] {
      yaclib_std::this_thread::sleep_for(10ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    auto f2 = yaclib::Run(tp, [] {
      return 2;
    });

    co_await Await(f1, f2);
    co_return std::move(f1).GetUnsafe().Ok() + std::move(f2).GetUnsafe().Ok();
  };
  auto f = coro(tp);
  EXPECT_EQ(std::move(f).Get().Ok(), 3);
  tp->HardStop();
  tp->Wait();
}

TEST(Await, JustWorksRange) {
  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<int> {
    std::array<yaclib::Future<int>, 2> arr;
    arr[0] = yaclib::Run(tp, [] {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    arr[1] = yaclib::Run(tp, [] {
      return 2;
    });

    co_await yaclib::Await(arr.begin(), 2);
    co_return std::move(arr[0]).GetUnsafe().Ok() + std::move(arr[1]).GetUnsafe().Ok();
  };
  auto future = coro(tp);
  EXPECT_EQ(std::move(future).Get().Ok(), 3);
  tp->HardStop();
  tp->Wait();
}

TEST(Await, CheckSuspend) {
  int counter = 0;
  auto tp = yaclib::MakeThreadPool();
  const auto coro_sleep_time = 50ms * YACLIB_CI_SLOWDOWN;
  auto was = std::chrono::steady_clock::now();

  auto coro = [&]() -> yaclib::Future<void> {
    counter = 1;
    auto future1 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });
    auto future2 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });

    co_await Await(future1, future2);
    counter = 2;
    co_return;
  };

  auto outer_future = coro();

  EXPECT_LT(std::chrono::steady_clock::now() - was, coro_sleep_time);
  EXPECT_EQ(1, counter);

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

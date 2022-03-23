#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
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

TEST(Await, CheckSuspend) {
#if defined(YACLIB_UBSAN) && (defined(__GLIBCPP__) || defined(__GLIBCXX__))
  // GTEST_SKIP();
#endif
  int counter = 0;
  auto tp = yaclib::MakeThreadPool(2);
  const auto coro_sleep_time = 50ms * YACLIB_CI_SLOWDOWN;
  auto was = std::chrono::steady_clock::now();
  std::atomic_bool barrier = false;

  auto coro = [&]() -> yaclib::Future<void> {
    counter = 1;
    auto future1 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });
    auto future2 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });

    std::cerr << "future1 %p" << &future1 << "; future2 %p" << &future2 << std::endl;
    co_await Await(future1, future2);
    std::cerr << "future1 %p" << &future1 << "; future2 %p" << &future2 << std::endl;

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

  std::cerr << "Get thread: " << std::this_thread::get_id() << std::endl;
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

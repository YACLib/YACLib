#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/coroutine/switch.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <array>
#include <exception>
#include <sstream>
#include <stack>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(Switch, JustWorks) {
  auto main_thread = yaclib_std::this_thread::get_id();

  std::cout << "MAIN THREAD = " << yaclib_std::this_thread::get_id() << '\n';

  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<void> {
    co_await yaclib::detail::SwitchAwaiter<void, yaclib::StopError>(tp);
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
    co_return;
  };
  auto f = coro(tp);
  std::ignore = std::move(f).Get();
  tp->HardStop();
  tp->Wait();
}

TEST(Switch, ManyCoros) {
  auto tp = yaclib::MakeThreadPool();

  yaclib_std::atomic_int32_t sum = 0;

  auto coro = [&](int a) -> yaclib::Future<void> {
    co_await yaclib::detail::SwitchAwaiter<void, yaclib::StopError>(tp);
    auto other_thread = yaclib_std::this_thread::get_id();

    std::stringstream ss;

    ss << "thread_id: " << yaclib_std::this_thread::get_id() << ", value: " << a << '\n';
    sum.fetch_add(a, std::memory_order_acquire);

    std::cout << ss.str();
    co_return;
  };

  const int N = 10;
  std::vector<yaclib::Future<void>> vec;
  vec.reserve(N);
  for (int i = 0; i < N; ++i) {
    vec.push_back(coro(i));
  }

  using namespace std::chrono_literals;
  yaclib_std::this_thread::sleep_for(10ms);  // kind of hard work

  for (auto& cor : vec) {
    std::ignore = std::move(cor).Get();
  }

  EXPECT_EQ((0 + 9) * 10 / 2, sum.fetch_add(0, std::memory_order_relaxed));

  tp->HardStop();
  tp->Wait();
}

TEST(Switch, Cancel) {
  using namespace std::chrono_literals;
  auto tp = yaclib::MakeThreadPool();

  yaclib_std::atomic_int32_t sum = 0;

  auto main_thread = yaclib_std::this_thread::get_id();

  auto coro = [&](int a) -> yaclib::Future<void> {

    yaclib_std::this_thread::sleep_for(10ms);


    co_await yaclib::detail::SwitchAwaiter<void, yaclib::StopError>(tp);
    auto other_thread = yaclib_std::this_thread::get_id();

    EXPECT_EQ(other_thread, main_thread);

    sum.fetch_add(a, std::memory_order_acquire);
    co_return;
  };

  tp->HardStop();

  const int N = 10;
  std::vector<yaclib::Future<void>> vec;
  vec.reserve(N);
  for (int i = 0; i < N; ++i) {
    vec.push_back(coro(i));
  }

  for (auto& cor : vec) {
    std::ignore = std::move(cor).Get();
  }

  ASSERT_EQ((0 + 9) * 10 / 2, sum.fetch_add(0, std::memory_order_relaxed));

  tp->Wait();
}

}  // namespace

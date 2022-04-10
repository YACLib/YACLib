#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/detail/via_awaiter.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/coroutine/via.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <exception>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(Via, JustWorks) {
  auto main_thread = yaclib_std::this_thread::get_id();

  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<void> {
    co_await Via(tp);
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
    co_return;
  };
  auto f = coro(tp);
  std::ignore = std::move(f).Get();
  tp->HardStop();
  tp->Wait();
}

TEST(Via, ManyCoros) {
  auto tp = yaclib::MakeThreadPool();

  yaclib_std::atomic_int32_t sum = 0;

  auto coro = [&](int a) -> yaclib::Future<void> {
    co_await Via(tp);
    auto other_thread = yaclib_std::this_thread::get_id();

    sum.fetch_add(a, std::memory_order_acquire);

    co_return;
  };

  const int N = 10;
  std::vector<yaclib::Future<void>> vec;
  vec.reserve(N);
  for (int i = 0; i < N; ++i) {
    vec.push_back(coro(i));
  }

  using namespace std::chrono_literals;

  for (auto& cor : vec) {
    std::ignore = std::move(cor).Get();
  }

  EXPECT_EQ((0 + 9) * 10 / 2, sum.fetch_add(0, std::memory_order_relaxed));

  tp->HardStop();
  tp->Wait();
}

TEST(Via, Cancel) {
  using namespace std::chrono_literals;
  auto tp = yaclib::MakeThreadPool();

  yaclib_std::atomic_int32_t sum = 0;

  auto main_thread = yaclib_std::this_thread::get_id();

  auto coro = [&](int a) -> yaclib::Future<void> {
    yaclib_std::this_thread::sleep_for(10ms);

    co_await Via(tp);
    auto other_thread = yaclib_std::this_thread::get_id();

    EXPECT_EQ(other_thread, main_thread);

    sum.fetch_add(a, std::memory_order_acquire);
    co_return;
  };

  tp->HardStop();

  constexpr std::size_t N = 10;
  std::vector<yaclib::Future<void>> vec;
  vec.reserve(N);
  for (std::size_t i = 0; i < N; ++i) {
    vec.push_back(coro(i));
  }

  for (auto& cor : vec) {
    EXPECT_FALSE(static_cast<bool>(std::move(cor).Get()));
  }

  ASSERT_EQ(0, sum.fetch_add(0, std::memory_order_relaxed));
  tp->Wait();
}

TEST(Via, LockWithStrand) {
  using namespace std::chrono_literals;
  auto tp = yaclib::MakeThreadPool();
  auto strand = yaclib::MakeStrand(tp);

  static const std::size_t kIncrements = 32768;
  std::size_t sum = 0;

  auto coro = [&](int a) -> yaclib::Future<void> {
    co_await Via(strand);  // lock
    ++sum;
    co_return;  // automatic unlocking
  };

  std::vector<yaclib::Future<void>> vec;
  vec.reserve(kIncrements);
  for (int i = 0; i < kIncrements; ++i) {
    vec.push_back(coro(i));
  }

  for (auto& cor : vec) {
    std::ignore = std::move(cor).Get();
  }

  ASSERT_EQ(kIncrements, sum);

  tp->HardStop();
  tp->Wait();
}

}  // namespace

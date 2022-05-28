#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future_traits.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/exe/strand.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <array>
#include <exception>
#include <utility>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(On, JustWorks) {
  auto main_thread = yaclib_std::this_thread::get_id();

  auto tp = yaclib::MakeThreadPool();
  auto coro = [&]() -> yaclib::Future<void> {
    co_await On(*tp);
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
    co_return;
  };
  auto f = coro();
  std::ignore = std::move(f).Get();
  tp->HardStop();
  tp->Wait();
}

// TODO(mkornaukhov03)
// Bad test, TSAN fails
TEST(On, ManyCoros) {
  auto main_thread = yaclib_std::this_thread::get_id();
  auto tp = yaclib::MakeThreadPool();
  yaclib_std::atomic_int32_t sum = 0;
  auto coro = [&](int a) -> yaclib::Future<void> {
    co_await On(*tp);
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
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
  yaclib_std::this_thread::sleep_for(10ms);  // kind of hard work

  for (auto& cor : vec) {
    std::ignore = std::move(cor).Get();
  }

  EXPECT_EQ((0 + 9) * 10 / 2, sum.fetch_add(0, std::memory_order_relaxed));

  tp->HardStop();
  tp->Wait();
}

// TODO(mkornaukhov03) Bad test, tasks are not submitted in thread pool
TEST(On, Drop) {
  using namespace std::chrono_literals;
  auto tp = yaclib::MakeThreadPool(1);
  tp->Stop();
  tp->Wait();

  yaclib_std::atomic_size_t sum = 0;

  auto main_thread = yaclib_std::this_thread::get_id();
  auto coro = [&](std::size_t a) -> yaclib::Future<void> {
    co_await On(*tp);
    auto curr_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(curr_thread, main_thread);
    sum.fetch_add(a, std::memory_order_relaxed);
    co_return;
  };

  constexpr std::size_t kSize = 10;
  std::array<yaclib::Future<void>, kSize> futures;
  for (std::size_t i = 0; i != kSize; ++i) {
    futures[i] = coro(i);
  }
  Wait(futures.data(), futures.size());
  ASSERT_EQ(0, sum.load());
}

TEST(On, LockWithStrand) {
  using namespace std::chrono_literals;

#if YACLIB_FAULT == 1
  constexpr std::size_t kIncrements = 1000;
#else
  constexpr std::size_t kIncrements = 10;
#endif
  const std::size_t kThreads = std::max(3U, std::thread::hardware_concurrency() / 2) - 1;

  auto tp = yaclib::MakeThreadPool(kThreads);
  auto strand = yaclib::MakeStrand(tp);

  std::size_t sum = 0;
  yaclib_std::atomic_bool end = true;
  auto inc = [&, mutex = strand](yaclib::IExecutor& thread) -> yaclib::Future<void> {
    co_await On(*mutex);  // lock
    end.store(true, std::memory_order_release);
    ++sum;
    co_await On(thread);  //  unlock
    co_return;
  };
  auto add_value = [&, t = tp](size_t increments) -> yaclib::Future<void> {
    co_await On(*t);  // schedule to thread pool
    std::vector<yaclib::Future<void>> vec;
    vec.reserve(increments);
    for (size_t i = 0; i != increments; ++i) {
      vec.push_back(inc(*t));
    }
    co_await Await(vec.begin(), vec.size());
    co_return;
  };

  std::vector<yaclib::Future<void>> vec;
  vec.reserve(kIncrements);
  for (size_t i = 0; i != kThreads; ++i) {
    vec.push_back(add_value(kIncrements));
  }
  Wait(vec.data(), vec.size());

  ASSERT_EQ(sum, kThreads * kIncrements);

  end.store(false, std::memory_order_release);
  for (size_t i = 0; i != kThreads; ++i) {
    add_value(kIncrements).Detach();
  }
  while (!end.load(std::memory_order_acquire)) {
  }
  tp->Stop();
  tp->Wait();
  fprintf(stderr, "min: %lu | sum: %lu | max %lu", kThreads * kIncrements, sum, 2 * kThreads * kIncrements);
  ASSERT_GT(sum, kThreads * kIncrements);
  ASSERT_LE(sum, 2 * kThreads * kIncrements);
}

}  // namespace

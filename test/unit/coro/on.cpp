#include <util/async_suite.hpp>
#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/current_executor.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/coro/yield.hpp>
#include <yaclib/exe/strand.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <array>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

TEST(On, JustWorks) {
  auto main_thread = yaclib_std::this_thread::get_id();

  yaclib::FairThreadPool tp;
  auto coro = [&]() -> yaclib::Future<> {
    co_await On(tp);
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
    co_return{};
  };
  auto f = coro();
  std::ignore = std::move(f).Get();
  tp.HardStop();
  tp.Wait();
}

TEST(On, ManyCoros) {
  auto main_thread = yaclib_std::this_thread::get_id();
  yaclib::FairThreadPool tp;
  yaclib_std::atomic_int32_t sum = 0;
  auto coro = [&](int a) -> yaclib::Future<> {
    co_await On(tp);
    co_await yaclib::kYield;
    yaclib::IExecutor* current1 = &co_await yaclib::CurrentExecutor();
    yaclib::IExecutor* current2 = &co_await yaclib::Yield();
    EXPECT_EQ(current1, current2);
    EXPECT_EQ(current1, &tp);
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
    sum.fetch_add(a, std::memory_order_acquire);
    co_return{};
  };

  const int N = 10;
  std::vector<yaclib::Future<>> vec;
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

  tp.HardStop();
  tp.Wait();
}

// TODO(mkornaukhov03) Bad test, tasks are not submitted in thread pool
TEST(On, Drop) {
  using namespace std::chrono_literals;
  yaclib::FairThreadPool tp{1};
  tp.Stop();
  tp.Wait();

  yaclib_std::atomic_size_t sum = 0;

  auto main_thread = yaclib_std::this_thread::get_id();
  auto coro = [&](std::size_t a) -> yaclib::Future<> {
    co_await On(tp);
    auto curr_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(curr_thread, main_thread);
    sum.fetch_add(a, std::memory_order_relaxed);
    co_return{};
  };

  constexpr std::size_t kSize = 10;
  std::array<yaclib::Future<>, kSize> futures;
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
  const std::size_t kThreads = std::max(3U, yaclib_std::thread::hardware_concurrency() / 2) - 1;

  yaclib::FairThreadPool tp{kThreads};
  auto strand = yaclib::MakeStrand(&tp);

  std::size_t sum = 0;
  yaclib_std::atomic_bool end = true;
  auto inc = [&, mutex = strand](yaclib::IExecutor& thread) -> yaclib::Future<> {
    co_await On(*mutex);  // lock
    end.store(true, std::memory_order_release);
    ++sum;
    co_await On(thread);  //  unlock
    co_return{};
  };
  auto add_value = [&](std::size_t increments) -> yaclib::Future<> {
    co_await On(tp);  // schedule to thread pool
    std::vector<yaclib::Future<>> vec;
    vec.reserve(increments);
    for (std::size_t i = 0; i != increments; ++i) {
      vec.push_back(inc(tp));
    }
    co_await Await(vec.begin(), vec.size());
    co_return{};
  };

  std::vector<yaclib::Future<>> vec;
  vec.reserve(kIncrements);
  for (std::size_t i = 0; i != kThreads; ++i) {
    vec.push_back(add_value(kIncrements));
  }
  Wait(vec.data(), vec.size());

  ASSERT_EQ(sum, kThreads * kIncrements);

  end.store(false, std::memory_order_release);
  for (std::size_t i = 0; i != kThreads; ++i) {
    add_value(kIncrements).Detach();
  }
  while (!end.load(std::memory_order_acquire)) {
    yaclib_std::this_thread::yield();
  }
  tp.Stop();
  tp.Wait();
  std::cerr << "min: " << kThreads * kIncrements << " | sum: " << sum << " | max " << 2 * kThreads * kIncrements
            << std::endl;
  ASSERT_GT(sum, kThreads * kIncrements);
  ASSERT_LE(sum, 2 * kThreads * kIncrements);
}

TYPED_TEST(AsyncSuite, OnStopped) {
  bool a = false;
  bool b = false;

  auto coro = [&]() -> typename TestFixture::Type {
    a = true;
    co_await On(MakeInline(yaclib::StopTag{}));
    b = true;
    co_return{};
  };

  auto outer_future = coro()
                        .ThenInline([](auto&& r) {
                          return std::move(r);
                        })
                        .ThenInline([] {
                        });
  EXPECT_EQ(std::move(outer_future).Get().State(), yaclib::ResultState::Error);

  EXPECT_TRUE(a);
  EXPECT_FALSE(b);
}

TYPED_TEST(AsyncSuite, Detach) {
  yaclib::FairThreadPool tp{1};
  int counter = 0;

  auto coro = [&]() -> typename TestFixture::Type {
    ++counter;
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(100ms);
    ++counter;
    co_return{};
  };
  coro().Detach();

  tp.Stop();
  tp.Wait();
  EXPECT_EQ(counter, 2);
}

}  // namespace
}  // namespace test

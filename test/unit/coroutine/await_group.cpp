#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/await_group.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/coroutine/on.hpp>
#include <yaclib/executor/manual.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <array>
#include <exception>
#include <stack>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

TEST(AwaitGroup, JustWorks) {
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

    yaclib::AwaitGroup await_group;
    await_group.Add(f1);
    await_group.Add(f2);

    co_await await_group;
    co_return std::move(f1).Touch().Ok() + std::move(f2).Touch().Ok();
  };
  auto f = coro(tp);
  EXPECT_EQ(std::move(f).Get().Ok(), 3);
  tp->HardStop();
  tp->Wait();
}

/* void Stress1(const std::size_t kWaiters, const std::size_t kWorkers, test::util::Duration dur) {
  auto tp = yaclib::MakeThreadPool();
  util::StopWatch sw;
  while (sw.Elapsed() < dur) {
    yaclib::AwaitGroup wg;

    yaclib_std::atomic_size_t waiters_done{0};
    yaclib_std::atomic_size_t workers_done{0};

    wg.Add(kWorkers);

    auto waiter = [&]() -> yaclib::Future<void> {
      co_await On(*tp);
      // co_await wg;
      waiters_done.fetch_add(1);
    };

    std::vector<yaclib::Future<void>> waiters(kWaiters);
    for (std::size_t i = 0; i < kWaiters; ++i) {
      waiters[i] = waiter();
    }

    auto worker = [&]() -> yaclib::Future<void> {
      co_await On(*tp);
      workers_done.fetch_add(1);
      wg.Done();
    };

    std::vector<yaclib::Future<void>> workers(kWorkers);
    for (std::size_t i = 0; i < kWorkers; ++i) {
      workers[i] = worker();
    }

    Wait(workers.begin(), workers.end());
    Wait(waiters.begin(), waiters.end());
    EXPECT_EQ(waiters_done.load(), kWaiters);
    EXPECT_EQ(workers_done.load(), kWorkers);
  }
  tp->HardStop();
  tp->Wait();
}

TEST(AwaitGroup, Stress) {
#if defined(YACLIB_UBSAN) && (defined(__GLIBCPP__) || defined(__GLIBCXX__))
  GTEST_SKIP();
#endif
  using namespace std::chrono_literals;
  constexpr std::array<std::tuple<std::size_t, std::size_t, test::util::Duration>, 1> cases = {
    std::tuple{1, 1, 1s},
  };  //                                                                                               {2, 2, 1s},
      // {16, 16, 1s}};
  for (auto&& [a, b, c] : cases) {
    Stress1(a, b, c);
  }
}
*/

TEST(AwaitGroup, OneWaiter) {
  auto manual = yaclib::MakeManual();

  yaclib::AwaitGroup wg;

  bool waiter_done = false;
  bool worker_done = false;

  auto waiter = [&]() -> yaclib::Future<void> {
    co_await On(*manual);

    co_await wg;

    EXPECT_TRUE(worker_done);
    waiter_done = true;
  };

  auto future_waiter = waiter();

  auto worker = [&]() -> yaclib::Future<void> {
    co_await On(*manual);

    for (size_t i = 0; i < 10; ++i) {
      co_await On(*manual);
    }

    worker_done = true;

    wg.Done();
  };

  wg.Add(1);
  auto future_worker = worker();

  manual->Drain();

  EXPECT_TRUE(waiter_done);
  EXPECT_TRUE(worker_done);
}

TEST(AwaitGroup, Workers) {
  auto scheduler = yaclib::MakeManual();

  yaclib::AwaitGroup wg;

  static constexpr std::size_t kWorkers = 3;
  static constexpr std::size_t kWaiters = 4;
  static constexpr std::size_t kYields = 10;

  std::size_t waiters_done = 0;
  std::size_t workers_done = 0;

  auto waiter = [&]() -> yaclib::Future<void> {
    co_await On(*scheduler);

    std::cout << "before waiting wg" << std::endl;
    co_await wg;
    std::cout << "after waiting wg" << std::endl;
    EXPECT_EQ(workers_done, kWorkers);
    ++waiters_done;
  };

  for (std::size_t i = 0; i < kWaiters; ++i) {
    std::move(waiter());
  }

  auto worker = [&]() -> yaclib::Future<void> {
    co_await On(*scheduler);

    for (std::size_t i = 0; i < kYields; ++i) {
      co_await On(*scheduler);
    }

    ++workers_done;

    wg.Done();
  };

  wg.Add(kWorkers);
  for (std::size_t j = 0; j < kWorkers; ++j) {
    std::move(worker());
  }
  
  std::cout << "Right before draining\n"; 
  std::size_t steps = scheduler->Drain();

  EXPECT_EQ(workers_done, kWorkers);
  // EXPECT_EQ(waiters_done, kWaiters);

  EXPECT_GE(steps, kWaiters + kWorkers * kYields);
}

}  // namespace
}  // namespace test

#include <util/helpers.hpp>
#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/current_executor.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

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
  yaclib::FairThreadPool tp;
  auto coro = [&]() -> yaclib::Future<int> {
    auto f1 = yaclib::Run(tp, [] {
      yaclib_std::this_thread::sleep_for(1ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    auto f2 = yaclib::Run(tp, [] {
      return 2;
    });

    yaclib::WaitGroup<> wg;
    wg.Attach(f1, f2);

    co_await wg;
    co_return std::move(f1).Touch().Ok() + std::move(f2).Touch().Ok();
  };
  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Ok(), 3);
  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, JustWorks2) {
  yaclib::FairThreadPool tp1{1};
  yaclib::FairThreadPool tp2{1};

  yaclib::WaitGroup<> wg{1};
  auto coro1 = [&]() -> yaclib::Future<> {
    co_await On(tp1);
    auto* current1 = &co_await yaclib::CurrentExecutor();
    EXPECT_EQ(current1, &tp1);
    co_await wg.AwaitSticky();
    auto* current2 = &co_await yaclib::CurrentExecutor();
    EXPECT_EQ(current1, &tp1);
    co_return{};
  };
  auto coro2 = [&]() -> yaclib::Future<> {
    Finally done = [&]() noexcept {
      wg.Done();
    };
    co_await On(tp2);
    yaclib_std::this_thread::sleep_for(10ms);
    co_return{};
  };

  coro1().Detach();
  coro2().Detach();

  tp1.HardStop();
  tp1.Wait();
  tp2.HardStop();
  tp2.Wait();
}

TEST(AwaitEvent, JustCall) {
  // Safe only with fair thread pool
  yaclib::FairThreadPool tp{1};
  yaclib::OneShotEvent event;

  auto coro1 = [&]() -> yaclib::Future<> {
    co_await On(tp);
    co_await event;
    co_return{};
  };
  auto coro2 = [&]() -> yaclib::Future<> {
    co_await On(tp);
    event.Call();
    co_return{};
  };

  auto f1 = coro1();
  auto f2 = coro2();
  yaclib::Wait(f1, f2);

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, OneWaiter) {
  auto manual = yaclib::MakeManual();

  yaclib::WaitGroup<> wg;

  bool waiter_done = false;
  bool worker_done = false;

  auto waiter = [&]() -> yaclib::Future<> {
    co_await On(*manual);

    co_await wg;

    EXPECT_TRUE(worker_done);
    waiter_done = true;
    co_return{};
  };

  auto future_waiter = waiter();

  auto worker = [&]() -> yaclib::Future<> {
    co_await On(*manual);

    for (std::size_t i = 0; i < 10; ++i) {
      co_await On(*manual);
    }

    worker_done = true;

    wg.Done();
    co_return{};
  };

  wg.Add(1);
  auto future_worker = worker();

  std::ignore = static_cast<yaclib::ManualExecutor&>(*manual).Drain();

  EXPECT_TRUE(waiter_done);
  EXPECT_TRUE(worker_done);
}

TEST(AwaitGroup, Workers) {
  auto manual = yaclib::MakeManual();

  yaclib::WaitGroup<> wg{0};

  static constexpr std::size_t kWorkers = 3;
  static constexpr std::size_t kWaiters = 4;
  static constexpr std::size_t kYields = 10;

  std::size_t waiters_done = 0;
  std::size_t workers_done = 0;

  auto waiter = [&]() -> yaclib::Future<> {
    co_await On(*manual);

    co_await wg;
    EXPECT_EQ(workers_done, kWorkers);
    ++waiters_done;
    co_return{};
  };

  for (std::size_t i = 0; i < kWaiters; ++i) {
    std::ignore = waiter();
  }

  auto worker = [&]() -> yaclib::Future<> {
    co_await On(*manual);

    for (std::size_t i = 0; i < kYields; ++i) {
      co_await On(*manual);
    }

    ++workers_done;

    wg.Done();
    co_return{};
  };

  wg.Add(kWorkers);
  for (std::size_t j = 0; j < kWorkers; ++j) {
    std::ignore = worker();
  }

  std::size_t steps = static_cast<yaclib::ManualExecutor&>(*manual).Drain();

  EXPECT_EQ(workers_done, kWorkers);
  EXPECT_EQ(waiters_done, kWaiters);

  EXPECT_GE(steps, kWaiters + kWorkers * kYields);
}

TEST(AwaitGroup, BlockingWait) {
  const static std::size_t HW_CONC = std::max(2u, yaclib_std::thread::hardware_concurrency());
  yaclib::FairThreadPool tp{HW_CONC};

  yaclib::WaitGroup<> wg;

  std::atomic<std::size_t> workers = 0;

  static const std::size_t kWorkers = HW_CONC - 1;

  wg.Add(kWorkers);

  auto waiter = [&]() -> yaclib::Future<> {
    co_await On(tp);

    co_await wg;
    EXPECT_EQ(workers.load(), kWorkers);
    co_return{};
  };

  auto worker = [&]() -> yaclib::Future<> {
    co_await On(tp);

    std::this_thread::sleep_for(0.5s * YACLIB_CI_SLOWDOWN);
    ++workers;
    wg.Done();
    co_return{};
  };

  util::StopWatch sw;

  auto waiter_future = waiter();
  std::vector<yaclib::Future<>> worker_futures(kWorkers);
  for (std::size_t i = 0; i < kWorkers; ++i) {
    worker_futures[i] = worker();
  }

  Wait(worker_futures.begin(), worker_futures.end());
  Wait(waiter_future);
  EXPECT_TRUE(sw.Elapsed() < .6s * YACLIB_CI_SLOWDOWN);
  tp.HardStop();
  tp.Wait();
}
TEST(AwaitGroup, WithFutures) {
  yaclib::FairThreadPool tp{4};

  yaclib::WaitGroup<> wg;

  yaclib_std::atomic_size_t done = 0;
  yaclib_std::atomic_bool flag = false;

  auto squarer = [&](int x) -> yaclib::Future<int> {
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(YACLIB_CI_SLOWDOWN * 1ms);
    done.fetch_add(1);
    while (!flag.load()) {
    }
    co_return x* x;
  };

  auto waiter = [&]() -> yaclib::Future<int> {
    auto one = squarer(1);
    auto four = squarer(2);
    auto nine = squarer(3);
    wg.Attach(one, four, nine);
    EXPECT_FALSE(flag.load());
    co_await wg.AwaitOn(tp);
    EXPECT_TRUE(flag.load());
    co_return std::move(one).Get().Value() + std::move(four).Get().Value() + std::move(nine).Get().Value();
  };

  auto res = waiter();
  EXPECT_FALSE(res.Ready());
  flag.store(true);

  EXPECT_EQ(1 + 2 * 2 + 3 * 3, std::move(res).Get().Value());

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, SwichThread) {
  yaclib::FairThreadPool tp{4};

  yaclib::WaitGroup<> wg;

  auto squarer = [&](int x) -> yaclib::Future<int> {
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(YACLIB_CI_SLOWDOWN * 10ms);
    co_return x + x;
  };

  auto main_thread = yaclib_std::this_thread::get_id();

  auto waiter = [&]() -> yaclib::Future<int> {
    auto one = squarer(1);
    wg.Attach(one);
    co_await wg.AwaitOn(tp);
    EXPECT_NE(main_thread, yaclib_std::this_thread::get_id());
    co_return std::move(one).Get().Value();
  };

  EXPECT_EQ(2, waiter().Get().Value());

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, NonCoroWait) {
  yaclib::FairThreadPool tp{4};

  yaclib::WaitGroup<> wg;
  yaclib_std::atomic_int cnt{0};

  auto coro = [&](int x) -> yaclib::Future<> {
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(YACLIB_CI_SLOWDOWN * 25ms);
    cnt.fetch_add(x, std::memory_order_relaxed);
    co_return{};
  };
  yaclib::Future<> f1, f2, f3;
  f1 = coro(1);
  f2 = coro(2);
  f3 = coro(3);
  wg.Attach(f1, f2, f3);

  wg.Wait();

  EXPECT_EQ(6, cnt.load(std::memory_order_relaxed));

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, Reset) {
  yaclib::FairThreadPool tp{1};

  yaclib::WaitGroup<> wg;

  auto squarer = [&](int x) -> yaclib::Future<int> {
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(YACLIB_CI_SLOWDOWN * 1ms);
    co_return x + x;
  };

  auto main_thread = yaclib_std::this_thread::get_id();

  auto waiter = [&](int x) -> yaclib::Future<int> {
    auto one = squarer(x);
    EXPECT_EQ(main_thread, yaclib_std::this_thread::get_id());
    wg.Attach(one);
    co_await wg.AwaitOn(tp);
    co_return std::move(one).Touch().Ok();
  };

  EXPECT_EQ(5 + 5, waiter(5).Get().Ok());

  wg.Reset();

  EXPECT_EQ(6 + 6, waiter(6).Get().Ok());

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, AwaitOn) {
  yaclib::WaitGroup<> wg{1};
  wg.Done();
  int count = 0;
  auto coro = [&]() -> yaclib::Future<> {
    ++count;
    co_await wg.AwaitOn(yaclib::MakeInline());
    ++count;
    co_await wg.AwaitOn(yaclib::MakeInline(yaclib::StopTag{}));
    ++count;
    co_return{};
  };
  EXPECT_THROW(std::ignore = coro().Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  EXPECT_EQ(count, 2);
}

TEST(AwaitGroup, Detach) {
  yaclib::FairThreadPool tp{1};
  int counter = 0;
  yaclib::WaitGroup<> wg;
  auto coro = [&]() -> yaclib::Future<> {
    ++counter;
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(100ms);
    ++counter;
    co_return{};
  };
  wg.Consume(coro());
  wg.Wait();

  tp.Stop();
  tp.Wait();
  EXPECT_EQ(counter, 2);
}

}  // namespace
}  // namespace test

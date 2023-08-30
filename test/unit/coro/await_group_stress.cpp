#include <util/helpers.hpp>
#include <util/time.hpp>

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
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
void Stress1(const std::size_t kWaiters, const std::size_t kWorkers, test::util::Duration dur) {
  yaclib::FairThreadPool tp;
  util::StopWatch sw;
  while (sw.Elapsed() < dur) {
    yaclib::WaitGroup<> wg;

    yaclib_std::atomic_size_t waiters_done{0};
    yaclib_std::atomic_size_t workers_done{0};

    wg.Add(kWorkers);

    auto waiter = [&]() -> yaclib::Future<> {
      co_await On(tp);
      co_await wg;
      waiters_done.fetch_add(1);
      co_return{};
    };

    std::vector<yaclib::Future<>> waiters(kWaiters);
    for (std::size_t i = 0; i < kWaiters; ++i) {
      waiters[i] = waiter();
    }

    auto worker = [&]() -> yaclib::Future<> {
      co_await On(tp);
      workers_done.fetch_add(1);
      wg.Done();
      co_return{};
    };

    std::vector<yaclib::Future<>> workers(kWorkers);
    for (std::size_t i = 0; i < kWorkers; ++i) {
      workers[i] = worker();
    }

    Wait(workers.begin(), workers.end());
    Wait(waiters.begin(), waiters.end());
    EXPECT_EQ(waiters_done.load(), kWaiters);
    EXPECT_EQ(workers_done.load(), kWorkers);
  }
  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, Stress1) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
  const std::size_t COROS[] = {1, 8};
  for (const auto waiters : COROS) {
    for (const auto workers : COROS) {
      Stress1(waiters, workers, 500ms);
    }
  }
}

class Goer {
 public:
  explicit Goer(yaclib::IExecutor& scheduler, yaclib::WaitGroup<>& wg) : scheduler_(scheduler), wg_(wg) {
  }

  void Start(std::size_t steps) {
    steps_left_ = steps;
    Step();
  }

  std::size_t Steps() const {
    return steps_made_;
  }

 private:
  yaclib::Future<> NextStep() {
    Finally done = [&]() noexcept {
      wg_.Done();
    };
    co_await On(scheduler_);
    Step();
    co_return{};
  }

  void Step() {
    if (steps_left_ == 0) {
      return;
    }

    ++steps_made_;
    --steps_left_;

    wg_.Add(1);
    NextStep().Detach();
  }

 private:
  yaclib::IExecutor& scheduler_;
  yaclib::WaitGroup<>& wg_;
  std::size_t steps_left_ = 0;
  std::size_t steps_made_ = 0;
};

void Stress2(util::Duration duration) {
  yaclib::FairThreadPool tp{4};

  std::size_t iter = 0;

  util::StopWatch sw;
  while (sw.Elapsed() < duration) {
    ++iter;

    bool done = false;

    auto tester = [&tp, &done, iter]() -> yaclib::Future<> {
      const std::size_t steps = 1 + iter % 3;

      yaclib::WaitGroup<> wg;

      Goer goer{tp, wg};
      goer.Start(steps);

      co_await wg;

      EXPECT_EQ(goer.Steps(), steps);
      EXPECT_TRUE(steps > 0);

      done = true;
      co_return{};
    };

    std::ignore = tester().Get();

    EXPECT_TRUE(done);
  }
  tp.HardStop();
  tp.Wait();
}

TEST(AwaitGroup, Stress2) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
  Stress2(1s);
}

}  // namespace
}  // namespace test

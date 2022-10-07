#include <util/cpu_time.hpp>
#include <util/time.hpp>

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <cstddef>
#include <stdexcept>
#include <thread>
#include <vector>
#include <yaclib_std/atomic>
#include <yaclib_std/chrono>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

const auto kCoresCount = [] {
  const auto cores_count{yaclib_std::thread::hardware_concurrency()};
  EXPECT_GT(cores_count, 1);
  return cores_count;
}();

TEST(FairThreadPool, JustWork) {
  auto tp = yaclib::MakeFairThreadPool(kCoresCount);
  bool ready = false;
  Submit(*tp, [&] {
    ready = true;
  });

  tp->Stop();
  tp->Wait();

  EXPECT_TRUE(ready);
}

TEST(FairThreadPool, ExecuteFrom) {
  yaclib::FairThreadPool tp{kCoresCount};
  bool done{false};
  auto task = [&] {
    Submit(tp /*CurrentExecutor()*/, [&] {
      done = true;
    });
  };
  Submit(tp, task);

  tp.SoftStop();
  tp.Wait();

  EXPECT_TRUE(done);
}

enum class StopType {
  SoftStop = 0,
  Stop,
  HardStop,
};

void AfterStopImpl(yaclib::FairThreadPool& tp, StopType stop_type, bool need_wait) {
  bool ready = false;

  if (need_wait || stop_type != StopType::SoftStop) {
    Submit(tp, [&] {
      ready = true;
      if (!need_wait) {
        yaclib_std::this_thread::sleep_for(1ms);
      }
    });
    if (need_wait && stop_type == StopType::HardStop) {
      yaclib_std::this_thread::sleep_for(10ms);
    }
  }

  switch (stop_type) {
    case StopType::SoftStop:
      tp.SoftStop();
      break;
    case StopType::Stop:
      tp.Stop();
      break;
    case StopType::HardStop:
      tp.HardStop();
      break;
  }

  if (need_wait) {
    tp.Wait();
    EXPECT_TRUE(ready);
  }
  Submit(tp, [] {
    FAIL();
  });

  if (!need_wait) {
    tp.Wait();
  }

  // check calling Wait multiple times
  tp.Wait();
}

TEST(FairThreadPool, AfterStop) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop, StopType::HardStop}) {
    for (auto need_wait : {true, false}) {
      for (auto cores : {1U, kCoresCount}) {
        yaclib::FairThreadPool tp{cores};
        AfterStopImpl(tp, stop_type, need_wait);
      }
    }
  }
}

void TwoThreadPool(yaclib::FairThreadPool& tp1, yaclib::FairThreadPool& tp2) {
  bool done1{false};
  bool done2{false};

  test::util::StopWatch stop_watch;

  Submit(tp1, [&] {
    Submit(tp2, [&] {
      done1 = true;
    });
    yaclib_std::this_thread::sleep_for(200ms);
  });

  Submit(tp2, [&] {
    Submit(tp1, [&] {
      done2 = true;
    });
    yaclib_std::this_thread::sleep_for(200ms);
  });

  tp1.SoftStop();
  tp2.SoftStop();
  tp1.Wait();
  tp2.Wait();
  EXPECT_TRUE(done1);
  EXPECT_TRUE(done2);
  EXPECT_LT(stop_watch.Elapsed(), 400ms);
}

TEST(FairThreadPool, TwoThreadPool) {
  for (auto cores : {1U, kCoresCount}) {
    for (auto cores2 : {1U, kCoresCount}) {
      yaclib::FairThreadPool tp1{cores}, tp2{cores2};
      TwoThreadPool(tp1, tp2);
    }
  }
}

void Join(yaclib::FairThreadPool& tp, StopType stop_type) {
  switch (stop_type) {
    case StopType::SoftStop:
      tp.SoftStop();
      break;
    case StopType::Stop:
      tp.Stop();
      break;
    case StopType::HardStop:
      EXPECT_NE(stop_type, StopType::HardStop);
      break;
  }
  tp.Wait();
}

void FIFO(yaclib::FairThreadPool& tp, StopType stop_type) {
  std::size_t next_task{0};

  constexpr std::size_t kTasks{256};
  for (std::size_t i = 0; i != kTasks; ++i) {
    Submit(tp, [i, &next_task] {
      EXPECT_EQ(next_task, i);
      ++next_task;
    });
  }
  Join(tp, stop_type);
  EXPECT_EQ(next_task, kTasks);
}

TEST(FairThreadPool, SingleThreadFIFO) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    yaclib::FairThreadPool tp{1};
    FIFO(tp, stop_type);
  }
}

void Exception(yaclib::FairThreadPool& tp, StopType stop_type) {
  int flag = 0;
  yaclib_std::atomic_bool check{false};
  Submit(tp, [&] {
    flag += 1;
    while (stop_type == StopType::Stop && !check.load()) {
    }
    Submit(tp, [&] {
      flag += 2;
    });
    throw std::runtime_error{"task failed"};
  });

  switch (stop_type) {
    case StopType::SoftStop:
      tp.SoftStop();
      break;
    case StopType::Stop:
      tp.Stop();
      check.store(true);
      break;
    case StopType::HardStop:
      EXPECT_NE(stop_type, StopType::HardStop);
      break;
  }

  tp.Wait();

  switch (stop_type) {
    case StopType::SoftStop:
      EXPECT_EQ(flag, 3);
      break;
    case StopType::Stop:
      // might fail if sleep_for isn't enough for stop waiting
      EXPECT_EQ(flag, 1);
      break;
    case StopType::HardStop:
      EXPECT_NE(stop_type, StopType::HardStop);
      break;
  }
}

TEST(FairThreadPool, Exception) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    for (auto cores : {1U, kCoresCount}) {
      yaclib::FairThreadPool tp{cores};
      Exception(tp, stop_type);
    }
  }
}

void ManyTask(yaclib::FairThreadPool& tp, StopType stop_type, std::size_t threads_count) {
  const std::size_t tasks{1024 * threads_count / 3};

  yaclib_std::atomic_size_t completed{0};
  for (std::size_t i = 0; i != tasks; ++i) {
    Submit(tp, [&completed] {
      completed.fetch_add(1, std::memory_order_relaxed);
    });
  }

  Join(tp, stop_type);

  EXPECT_EQ(completed, tasks);
}

TEST(FairThreadPool, ManyTask) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    for (auto cores : {1U, kCoresCount}) {
      yaclib::FairThreadPool tp{cores};
      ManyTask(tp, stop_type, cores);
    }
  }
}

void UseAllThreads(yaclib::FairThreadPool& tp, StopType stop_type) {
  yaclib_std::atomic_size_t counter{0};

  auto sleeper = [&counter] {
    auto point = yaclib_std::chrono::steady_clock::now() + 50ms * YACLIB_CI_SLOWDOWN;
    do {
      yaclib_std::this_thread::sleep_until(point);
    } while (point > yaclib_std::chrono::steady_clock::now());  // Workaround for MinGW
    counter.fetch_add(1, std::memory_order_relaxed);
  };

  test::util::StopWatch stop_watch;

  Submit(tp, sleeper);
  Submit(tp, sleeper);

  Join(tp, stop_type);

  auto elapsed = stop_watch.Elapsed();

  EXPECT_EQ(counter, 2);
  EXPECT_GE(elapsed, 50ms * YACLIB_CI_SLOWDOWN);
  EXPECT_LT(elapsed, 100ms * YACLIB_CI_SLOWDOWN);
}

TEST(FairThreadPool, UseAllThreads) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    yaclib::FairThreadPool tp{2};
    UseAllThreads(tp, stop_type);
  }
}

void NotSequentialAndParallel(yaclib::FairThreadPool& tp, StopType stop_type) {
  // Not sequential start and parallel running
  yaclib_std::atomic_int counter{0};

  Submit(tp, [&] {
    yaclib_std::this_thread::sleep_for(300ms);
    counter.store(2, std::memory_order_release);
  });

  Submit(tp, [&] {
    counter.store(1, std::memory_order_release);
  });

  yaclib_std::this_thread::sleep_for(100ms);

  EXPECT_EQ(counter.load(std::memory_order_acquire), 1);

  Join(tp, stop_type);

  EXPECT_EQ(counter.load(std::memory_order_acquire), 2);
}

TEST(FairThreadPool, NotSequentialAndParallel) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    yaclib::FairThreadPool tp{2};
    NotSequentialAndParallel(tp, stop_type);
  }
}

// TODO(kononovk) Update this test, after CurrentThreadPool will be implemented
void Current(yaclib::FairThreadPool& tp) {
  // EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  Submit(tp, [&] {
    // EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
    yaclib_std::this_thread::sleep_for(10ms);
    // EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
  });

  // EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  yaclib_std::this_thread::sleep_for(1ms);

  // EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  tp.Stop();

  // EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  tp.Wait();

  // EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());
}

TEST(FairtThreadPool, Current) {
  for (auto cores : {1U, kCoresCount}) {
    yaclib::FairThreadPool tp{cores};
    Current(tp);
  }
}

void Lifetime(yaclib::FairThreadPool& tp, std::size_t threads) {
  class Task final {
   public:
    Task(Task&&) = default;
    Task(const Task&) = delete;
    Task& operator=(Task&&) = delete;
    Task& operator=(const Task&) = delete;

    explicit Task(yaclib_std::atomic<int>& counter) : _counter{counter} {
    }

    ~Task() {
      if (_done) {
        _counter.fetch_add(1, std::memory_order_release);
      }
    }

    void operator()() {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      _done = true;
    }

   private:
    bool _done{false};
    yaclib_std::atomic_int& _counter;
  };

  yaclib_std::atomic_int dead{0};

  for (std::size_t i = 0; i != threads; ++i) {
    Submit(tp, Task(dead));
  }

  yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN * threads / 2);
  EXPECT_EQ(dead.load(std::memory_order_acquire), threads);

  tp.Stop();
  tp.Wait();

  EXPECT_EQ(dead.load(), threads);
}

TEST(FairThreadPool, Lifetime) {
  yaclib::FairThreadPool tp{4};
  Lifetime(tp, 4);
}

void RacyCounter() {
#if defined(GTEST_OS_WINDOWS) && YACLIB_FAULT == 1
  GTEST_SKIP();  // Too long
#endif
  yaclib::FairThreadPool tp{2 * kCoresCount};

  yaclib_std::atomic<std::size_t> counter1{0};
  yaclib_std::atomic<std::size_t> counter2{0};

  static const std::size_t kIncrements = 123456;
  for (std::size_t i = 0; i < kIncrements; ++i) {
    Submit(tp, [&] {
      auto old = counter1.load(std::memory_order_relaxed);
      counter2.fetch_add(1, std::memory_order_relaxed);
      yaclib_std::this_thread::yield();
      counter1.store(old + 1, std::memory_order_relaxed);
    });
  }

  tp.Stop();
  tp.Wait();

  EXPECT_LT(counter1.load(), kIncrements);
  EXPECT_EQ(counter2.load(), kIncrements);
}

TEST(FairThreadPool, RacyCounter) {
  RacyCounter();
}

TEST(Manual, Alive) {
  yaclib::ManualExecutor manual;
  EXPECT_TRUE(manual.Alive());
}

TEST(InlineCall, Alive) {
  auto& inline_call = yaclib::MakeInline();
  EXPECT_EQ(inline_call.Tag(), yaclib::IExecutor::Type::Inline);
  EXPECT_TRUE(inline_call.Alive());
}

TEST(InlineDrop, Alive) {
  auto& inline_drop = yaclib::MakeInline(yaclib::StopTag{});
  EXPECT_FALSE(inline_drop.Alive());
}

// TODO(Ri7ay) Don't work on windows, check this:
//  https://stackoverflow.com/questions/12606033/computing-cpu-time-in-c-on-windows
#if YACLIB_CI_SLOWDOWN == 1 && (defined(GTEST_OS_LINUX) || defined(GTEST_OS_MAC))
void NotBurnCPU(yaclib::FairThreadPool& tp, std::size_t threads) {
  // Warmup
  for (std::size_t i = 0; i != threads; ++i) {
    if (i == threads - 1) {
      Submit(tp, [&tp] {
        yaclib_std::this_thread::sleep_for(100ms);
        tp.Stop();
      });
    } else {
      Submit(tp, [] {
      });
    }
  }

  test::util::ProcessCPUTimer cpu_timer;

  EXPECT_TRUE(tp.Alive());
  tp.Wait();

  EXPECT_LT(cpu_timer.Elapsed(), 50ms);
}

TEST(FairThreadPool, NotBurnCPU) {
  for (auto cores : {1U, kCoresCount}) {
    yaclib::FairThreadPool tp{cores};
    NotBurnCPU(tp, cores);
  }
}

#endif

}  // namespace
}  // namespace test

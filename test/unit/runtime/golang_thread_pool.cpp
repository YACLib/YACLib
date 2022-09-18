#include <util/cpu_time.hpp>
#include <util/time.hpp>

#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/golang_thread_pool.hpp>

#include <chrono>
#include <iostream>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

const auto kCoresCount = [] {
  auto const cores_count{yaclib_std::thread::hardware_concurrency()};
  EXPECT_GT(cores_count, 1);
  return cores_count;
}();

TEST(ThreadPool, JustWorks) {
  yaclib::GolangThreadPool tp{2};

  bool executed = false;
  yaclib::Submit(tp, [&] {
    executed = true;
    std::cout << "Hi!" << std::endl;
    tp.Stop();
  });
  tp.Wait();
  EXPECT_TRUE(executed);
}

TEST(ThreadPool, LocalPushing) {
  using namespace std::chrono_literals;

  yaclib::GolangThreadPool tp{2};

  yaclib::Submit(tp, [&] {
    yaclib_std::this_thread::sleep_for(2s);
  });
  yaclib::Submit(tp, [&] {
    // Task pushed in current thread
    yaclib::Submit(tp, [&, current_thread = yaclib_std::this_thread::get_id()] {
      auto stealing_thread = yaclib_std::this_thread::get_id();
      EXPECT_EQ(stealing_thread, current_thread);
      tp.Stop();
    });
  });
  tp.Wait();
}

TEST(ThreadPool, Stealing) {
  yaclib::GolangThreadPool tp{2};

  yaclib::Submit(tp, [&] {
    // Task pushed in current thread
    yaclib::Submit(tp, [&, current_thread = yaclib_std::this_thread::get_id()] {
      auto stealing_thread = yaclib_std::this_thread::get_id();
      EXPECT_NE(stealing_thread, current_thread);
      tp.Stop();
    });
    yaclib_std::this_thread::sleep_for(2s);
  });
  yaclib_std::this_thread::sleep_for(1s);
  yaclib::Submit(tp, [&] {
  });
  tp.Wait();
}

TEST(Golang, JustWork) {
  yaclib::GolangThreadPool tp{kCoresCount};

  bool ready = false;
  Submit(tp, [&] {
    ready = true;
  });

  tp.Stop();
  tp.Wait();

  EXPECT_TRUE(ready);
}

TEST(Golang, ExecuteFrom) {
  yaclib::GolangThreadPool tp{kCoresCount};
  bool done{false};
  auto task = [&] {
    Submit(tp, [&] {
      done = true;
    });
  };
  Submit(tp, task);

  tp.Stop();
  tp.Wait();

  EXPECT_TRUE(done);
}

TEST(Golang, TwoThreadPool) {
  yaclib::GolangThreadPool tp1{kCoresCount};
  yaclib::GolangThreadPool tp2{kCoresCount};

  bool done1{false};
  bool done2{false};

  test::util::StopWatch stop_watch;

  Submit(tp1, [&] {
    Submit(tp2, [&] {
      done1 = true;
      tp1.Cancel();
    });
    yaclib_std::this_thread::sleep_for(200ms);
  });

  Submit(tp2, [&] {
    Submit(tp1, [&] {
      done2 = true;
      tp2.Cancel();
    });
    yaclib_std::this_thread::sleep_for(200ms);
  });

  tp1.Wait();
  tp2.Wait();
  EXPECT_TRUE(done1);
  EXPECT_TRUE(done2);
  EXPECT_LT(stop_watch.Elapsed(), 400ms);
}

TEST(Golang, ExceptionStop) {
  yaclib::GolangThreadPool tp{kCoresCount};

  int flag = 0;
  Submit(tp, [&] {
    flag += 1;
    Submit(tp, [&] {
      flag += 2;
      tp.Stop();
    });
    throw std::runtime_error{"task failed"};
  });
  tp.Wait();
  EXPECT_EQ(flag, 3);
}

TEST(Golang, ManyTask) {
  yaclib::GolangThreadPool tp{kCoresCount};

  const std::size_t tasks{1024 * kCoresCount / 3};

  yaclib_std::atomic_size_t completed{0};
  for (std::size_t i = 0; i != tasks; ++i) {
    Submit(tp, [&completed] {
      completed.fetch_add(1, std::memory_order_relaxed);
    });
  }

  tp.Stop();
  tp.Wait();

  EXPECT_EQ(completed, tasks);
}

TEST(Golang, UseAllThreads) {
  yaclib::GolangThreadPool tp{2};

  yaclib_std::atomic_size_t counter{0};

  auto sleeper = [&counter] {
    auto point = yaclib_std::chrono::steady_clock::now() + 500ms * YACLIB_CI_SLOWDOWN;
    do {
      yaclib_std::this_thread::sleep_until(point);
    } while (point > yaclib_std::chrono::steady_clock::now());  // Workaround for MinGW
    counter.fetch_add(1, std::memory_order_relaxed);
  };

  test::util::StopWatch stop_watch;

  Submit(tp, sleeper);
  Submit(tp, sleeper);

  tp.Stop();
  tp.Wait();

  auto elapsed = stop_watch.Elapsed();

  EXPECT_EQ(counter, 2);
  EXPECT_GE(elapsed, 500ms * YACLIB_CI_SLOWDOWN);
  EXPECT_LT(elapsed, 1000ms * YACLIB_CI_SLOWDOWN);
}

TEST(Golang, NotSequentialAndParallel) {
  yaclib::GolangThreadPool tp{2};

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

  tp.Stop();
  tp.Wait();

  EXPECT_EQ(counter.load(std::memory_order_acquire), 2);
}

TEST(Golang, Current) {
  yaclib::GolangThreadPool tp{2};

  EXPECT_EQ(&tp, &yaclib::MakeInline());

  Submit(tp, [&] {
    //EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
    yaclib_std::this_thread::sleep_for(10ms);
    // EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
  });

  //EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  yaclib_std::this_thread::sleep_for(1ms);

  // EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  tp.Stop();

  //EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  tp.Wait();

  //EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());
}

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

TEST(Golang, Lifetime) {
  auto const threads = 4;
  yaclib::GolangThreadPool tp{threads};
  yaclib_std::atomic_int dead{0};

  for (std::size_t i = 0; i != 4; ++i) {
    Submit(tp, Task{dead});
  }

  yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN * threads / 2);

  tp.Stop();
  tp.Wait();

  EXPECT_EQ(dead.load(), threads);
}

TEST(Golang, RacyCounter) {
#if defined(GTEST_OS_WINDOWS) && YACLIB_FAULT == 1
  GTEST_SKIP();  // Too long
#endif
  yaclib::GolangThreadPool tp{kCoresCount};

  yaclib_std::atomic_size_t counter1{0};
  yaclib_std::atomic_size_t counter2{0};

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

}  // namespace
}  // namespace test

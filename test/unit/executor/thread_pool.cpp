#include <util/cpu_time.hpp>
#include <util/time.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <initializer_list>
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
  auto const cores_count{std::thread::hardware_concurrency()};
  EXPECT_GT(cores_count, 1);
  return cores_count;
}();

enum class FactoryTag {
  Light = 0,
  Heavy,
};

enum class ThreadPoolTag {
  Single = 0,
  Multi,
};

class Thread : public ::testing::Test {
 protected:
  void TearDown() override {
    _tps.clear();
  }

  void PopTP() {
    _tps.pop_back();
  }

  std::vector<yaclib::IThreadPoolPtr> _tps;
  yaclib::IThreadFactoryPtr _factory;
  FactoryTag _factory_tag{};
  ThreadPoolTag _tp_tag{};
};

class SingleThread : public Thread {
 protected:
  void PushTP() {
    _tps.push_back(MakeThreadPool(1, _factory));
  }
};

class MultiThread : public Thread {
 protected:
  void PushTP() {
    _tps.push_back(MakeThreadPool(kCoresCount, _factory));
  }
};

class SingleLightThread : public SingleThread {
 protected:
  void SetUp() override {
    _factory_tag = FactoryTag::Light;
    _tp_tag = ThreadPoolTag::Single;
    _factory = yaclib::MakeThreadFactory();
    PushTP();
  }
};

class SingleHeavyThread : public SingleThread {
 protected:
  void SetUp() override {
    _factory_tag = FactoryTag::Heavy;
    _tp_tag = ThreadPoolTag::Single;
    _factory = yaclib::MakeThreadFactory(1);
    PushTP();
  }
};

class MultiLightThread : public MultiThread {
 protected:
  void SetUp() override {
    _factory_tag = FactoryTag::Light;
    _tp_tag = ThreadPoolTag::Multi;
    _factory = yaclib::MakeThreadFactory();
    PushTP();
  }
};

class MultiHeavyThread : public MultiThread {
 protected:
  void SetUp() override {
    _factory_tag = FactoryTag::Heavy;
    _tp_tag = ThreadPoolTag::Multi;
    _factory = yaclib::MakeThreadFactory(kCoresCount);
    PushTP();
  }
};

void JustWork(yaclib::IThreadPoolPtr& tp) {
  bool ready = false;
  Submit(*tp, [&] {
    ready = true;
  });

  tp->Stop();
  tp->Wait();

  EXPECT_TRUE(ready);
}

TEST_F(SingleLightThread, JustWork) {
  EXPECT_EQ(yaclib::MakeInline().Tag(), yaclib::IExecutor::Type::Inline);
  JustWork(_tps[0]);
}
TEST_F(SingleHeavyThread, JustWork) {
  JustWork(_tps[0]);
}
TEST_F(MultiLightThread, JustWork) {
  JustWork(_tps[0]);
}
TEST_F(MultiHeavyThread, JustWork) {
  JustWork(_tps[0]);
}

void ExecuteFrom(yaclib::IThreadPoolPtr& tp) {
  bool done{false};
  auto task = [&] {
    Submit(yaclib::CurrentThreadPool(), [&] {
      done = true;
    });
  };
  Submit(*tp, task);

  tp->SoftStop();
  tp->Wait();

  EXPECT_TRUE(done);
}

TEST_F(SingleLightThread, ExecuteFrom) {
  ExecuteFrom(_tps[0]);
}
TEST_F(SingleHeavyThread, ExecuteFrom) {
  ExecuteFrom(_tps[0]);
}
TEST_F(MultiLightThread, ExecuteFrom) {
  ExecuteFrom(_tps[0]);
}
TEST_F(MultiHeavyThread, ExecuteFrom) {
  ExecuteFrom(_tps[0]);
}

enum class StopType {
  SoftStop = 0,
  Stop,
  HardStop,
};

void AfterStopImpl(yaclib::IThreadPoolPtr& tp, StopType stop_type, bool need_wait) {
  bool ready = false;

  if (need_wait || stop_type != StopType::SoftStop) {
    Submit(*tp, [&] {
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
      tp->SoftStop();
      break;
    case StopType::Stop:
      tp->Stop();
      break;
    case StopType::HardStop:
      tp->HardStop();
      break;
  }

  if (need_wait) {
    tp->Wait();
    EXPECT_TRUE(ready);
  }
  Submit(*tp, [] {
    FAIL();
  });

  if (!need_wait) {
    tp->Wait();
  }
}

TEST_F(SingleLightThread, AfterStop) {
  PopTP();
  for (auto stop_type : {StopType::SoftStop, StopType::Stop, StopType::HardStop}) {
    for (auto need_wait : {true, false}) {
      PushTP();
      AfterStopImpl(_tps[0], stop_type, need_wait);
      PopTP();
    }
  }
}
TEST_F(SingleHeavyThread, AfterStop) {
  PopTP();
  for (auto stop_type : {StopType::SoftStop, StopType::Stop, StopType::HardStop}) {
    for (auto need_wait : {true, false}) {
      PushTP();
      AfterStopImpl(_tps[0], stop_type, need_wait);
      PopTP();
    }
  }
}
TEST_F(MultiLightThread, AfterStop) {
  PopTP();
  for (auto stop_type : {StopType::SoftStop, StopType::Stop, StopType::HardStop}) {
    for (auto need_wait : {true, false}) {
      PushTP();
      AfterStopImpl(_tps[0], stop_type, need_wait);
      PopTP();
    }
  }
}
TEST_F(MultiHeavyThread, AfterStop) {
  PopTP();
  for (auto stop_type : {StopType::SoftStop, StopType::Stop, StopType::HardStop}) {
    for (auto need_wait : {true, false}) {
      PushTP();
      AfterStopImpl(_tps[0], stop_type, need_wait);
      PopTP();
    }
  }
}

void TwoThreadPool(yaclib::IThreadPoolPtr& tp1, yaclib::IThreadPoolPtr& tp2) {
  bool done1{false};
  bool done2{false};

  test::util::StopWatch stop_watch;

  Submit(*tp1, [&] {
    Submit(*tp2, [&] {
      done1 = true;
    });
    yaclib_std::this_thread::sleep_for(200ms);
  });

  Submit(*tp2, [&] {
    Submit(*tp1, [&] {
      done2 = true;
    });
    yaclib_std::this_thread::sleep_for(200ms);
  });

  tp1->SoftStop();
  tp2->SoftStop();
  tp1->Wait();
  tp2->Wait();
  EXPECT_TRUE(done1);
  EXPECT_TRUE(done2);
  EXPECT_LT(stop_watch.Elapsed(), 400ms);
}

TEST_F(SingleLightThread, TwoThreadPool) {
  PushTP();
  TwoThreadPool(_tps[0], _tps[1]);
}
TEST_F(SingleHeavyThread, TwoThreadPool) {
  PushTP();
  TwoThreadPool(_tps[0], _tps[1]);
}
TEST_F(MultiLightThread, TwoThreadPool) {
  PushTP();
  TwoThreadPool(_tps[0], _tps[1]);
}
TEST_F(MultiHeavyThread, TwoThreadPool) {
  PushTP();
  TwoThreadPool(_tps[0], _tps[1]);
}

void Join(yaclib::IThreadPoolPtr& tp, StopType stop_type) {
  switch (stop_type) {
    case StopType::SoftStop:
      tp->SoftStop();
      break;
    case StopType::Stop:
      tp->Stop();
      break;
    case StopType::HardStop:
      EXPECT_NE(stop_type, StopType::HardStop);
      break;
  }
  tp->Wait();
}

void FIFO(yaclib::IThreadPoolPtr& tp, StopType stop_type) {
  std::size_t next_task{0};

  constexpr std::size_t kTasks{256};
  for (std::size_t i = 0; i != kTasks; ++i) {
    Submit(*tp, [i, &next_task] {
      EXPECT_EQ(next_task, i);
      ++next_task;
    });
  }
  Join(tp, stop_type);
  EXPECT_EQ(next_task, kTasks);
}

TEST_F(SingleLightThread, FIFO) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    FIFO(_tps[0], stop_type);
    PopTP();
    PushTP();
  }
}

TEST_F(SingleHeavyThread, FIFO) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    FIFO(_tps[0], stop_type);
    PopTP();
    PushTP();
  }
}

void Exception(yaclib::IThreadPoolPtr& tp, StopType stop_type) {
  int flag = 0;
  Submit(*tp, [&] {
    flag += 1;
    std::this_thread::sleep_for(1ms);  // Wait stop
    Submit(*tp, [&] {
      flag += 2;
    });
    throw std::runtime_error{"task failed"};
  });

  switch (stop_type) {
    case StopType::SoftStop:
      tp->SoftStop();
      break;
    case StopType::Stop:
      tp->Stop();
      break;
    case StopType::HardStop:
      EXPECT_NE(stop_type, StopType::HardStop);
      break;
  }

  tp->Wait();

  switch (stop_type) {
    case StopType::SoftStop:
      EXPECT_EQ(flag, 3);
      break;
    case StopType::Stop:
      EXPECT_EQ(flag, 1);
      break;
    case StopType::HardStop:
      EXPECT_NE(stop_type, StopType::HardStop);
      break;
  }
}

TEST_F(SingleLightThread, Exception) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    Exception(_tps[0], stop_type);
    PopTP();
    PushTP();
  }
}
TEST_F(SingleHeavyThread, Exception) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    Exception(_tps[0], stop_type);
    PopTP();
    PushTP();
  }
}

void ManyTask(yaclib::IThreadPoolPtr& tp, StopType stop_type, std::size_t threads_count) {
  const std::size_t tasks{1024 * threads_count / 3};

  yaclib_std::atomic_size_t completed{0};
  for (std::size_t i = 0; i != tasks; ++i) {
    Submit(*tp, [&completed] {
      completed.fetch_add(1, std::memory_order_relaxed);
    });
  }

  Join(tp, stop_type);

  EXPECT_EQ(completed, tasks);
}

TEST_F(SingleLightThread, ManyTask) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    ManyTask(_tps[0], stop_type, 1);
    PopTP();
    PushTP();
  }
}
TEST_F(SingleHeavyThread, ManyTask) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    ManyTask(_tps[0], stop_type, 1);
    PopTP();
    PushTP();
  }
}
TEST_F(MultiLightThread, ManyTask) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    ManyTask(_tps[0], stop_type, kCoresCount);
    PopTP();
    PushTP();
  }
}
TEST_F(MultiHeavyThread, ManyTask) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    ManyTask(_tps[0], stop_type, kCoresCount);
    PopTP();
    PushTP();
  }
}

void UseAllThreads(yaclib::IThreadPoolPtr& tp, StopType stop_type) {
  yaclib_std::atomic_size_t counter{0};

  auto sleeper = [&counter] {
    auto point = yaclib_std::chrono::steady_clock::now() + 50ms * YACLIB_CI_SLOWDOWN;
    do {
      yaclib_std::this_thread::sleep_until(point);
    } while (point > yaclib_std::chrono::steady_clock::now());  // Workaround for MinGW
    counter.fetch_add(1, std::memory_order_relaxed);
  };

  test::util::StopWatch stop_watch;

  Submit(*tp, sleeper);
  Submit(*tp, sleeper);

  Join(tp, stop_type);

  auto elapsed = stop_watch.Elapsed();

  EXPECT_EQ(counter, 2);
  EXPECT_GE(elapsed, 50ms * YACLIB_CI_SLOWDOWN);
  EXPECT_LT(elapsed, 100ms * YACLIB_CI_SLOWDOWN);
}

TEST_F(MultiLightThread, UseAllThreads) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = yaclib::MakeThreadPool(2);
    UseAllThreads(_tps[0], stop_type);
  }
}
TEST_F(MultiHeavyThread, UseAllThreads) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = yaclib::MakeThreadPool(2);
    UseAllThreads(_tps[0], stop_type);
  }
}

void NotSequentialAndParallel(yaclib::IThreadPoolPtr& tp, StopType stop_type) {
  // Not sequential start and parallel running
  yaclib_std::atomic_int counter{0};

  Submit(*tp, [&] {
    yaclib_std::this_thread::sleep_for(300ms);
    counter.store(2, std::memory_order_release);
  });

  Submit(*tp, [&] {
    counter.store(1, std::memory_order_release);
  });

  yaclib_std::this_thread::sleep_for(100ms);

  EXPECT_EQ(counter.load(std::memory_order_acquire), 1);

  Join(tp, stop_type);

  EXPECT_EQ(counter.load(std::memory_order_acquire), 2);
}

TEST_F(MultiLightThread, NotSequentialAndParallel) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = yaclib::MakeThreadPool(2);
    NotSequentialAndParallel(_tps[0], stop_type);
  }
}
TEST_F(MultiHeavyThread, NotSequentialAndParallel) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = yaclib::MakeThreadPool(2);
    NotSequentialAndParallel(_tps[0], stop_type);
  }
}

void Current(yaclib::IThreadPoolPtr& tp) {
  EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  Submit(*tp, [&] {
    EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
    yaclib_std::this_thread::sleep_for(10ms);
    EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
  });

  EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());

  yaclib_std::this_thread::sleep_for(1ms);

#if !defined(YACLIB_FIBER)
  EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());
#endif

  tp->Stop();

#if !defined(YACLIB_FIBER)
  EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());
#endif

  tp->Wait();

  EXPECT_EQ(&yaclib::CurrentThreadPool(), &yaclib::MakeInline());
}

TEST_F(SingleLightThread, Current) {
  Current(_tps[0]);
}
TEST_F(SingleHeavyThread, Current) {
  Current(_tps[0]);
}

// TODO(myannyax) racey for fibers... maybe change hardware concurrency to 1 for fibers?
#if !defined(YACLIB_FIBER)
TEST_F(MultiLightThread, Current) {
  Current(_tps[0]);
}
TEST_F(MultiHeavyThread, Current) {
  Current(_tps[0]);
}
#endif

void Lifetime(yaclib::IThreadPoolPtr& tp, std::size_t threads) {
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
    Submit(*tp, Task(dead));
  }

  yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN * threads / 2);
  EXPECT_EQ(dead.load(std::memory_order_acquire), threads);

  tp->Stop();
  tp->Wait();

  EXPECT_EQ(dead.load(), threads);
}

TEST_F(MultiLightThread, Lifetime) {
  _tps[0] = nullptr;  // Release threads
  _tps[0] = yaclib::MakeThreadPool(4);
  Lifetime(_tps[0], 4);
}
TEST_F(MultiHeavyThread, Lifetime) {
  _tps[0] = nullptr;  // Release threads
  _tps[0] = yaclib::MakeThreadPool(4);
  Lifetime(_tps[0], 4);
}

void RacyCounter(yaclib::IThreadFactoryPtr& factory) {
  auto tp = MakeThreadPool(2 * kCoresCount, factory);

  yaclib_std::atomic<size_t> counter1{0};
  yaclib_std::atomic<size_t> counter2{0};

  static const std::size_t kIncrements = 123456;
  for (std::size_t i = 0; i < kIncrements; ++i) {
    Submit(*tp, [&] {
      auto old = counter1.load(std::memory_order_relaxed);
      counter2.fetch_add(1, std::memory_order_relaxed);
      yaclib_std::this_thread::yield();
      counter1.store(old + 1, std::memory_order_relaxed);
    });
  }

  tp->Stop();
  tp->Wait();

  EXPECT_LT(counter1.load(), kIncrements);
  EXPECT_EQ(counter2.load(), kIncrements);
}

TEST_F(MultiLightThread, RacyCounter) {
  PopTP();
  RacyCounter(_factory);
}
TEST_F(MultiHeavyThread, RacyCounter) {
  PopTP();
  RacyCounter(_factory);
}

/// TODO(Ri7ay): Don't work on windows, check this:
///  https://stackoverflow.com/questions/12606033/computing-cpu-time-in-c-on-windows
#if defined(__linux)

void NotBurnCPU(yaclib::IThreadPoolPtr& tp, std::size_t threads) {
  // Warmup
  for (std::size_t i = 0; i != threads; ++i) {
    Submit(*tp, [&] {
      yaclib_std::this_thread::sleep_for(100ms);
    });
  }

  test::util::ProcessCPUTimer cpu_timer;

  tp->Stop();
  tp->Wait();

  EXPECT_LT(cpu_timer.Elapsed(), 50ms);
}

TEST_F(SingleLightThread, NotBurnCPU) {
  NotBurnCPU(_tps[0], 1);
}
TEST_F(SingleHeavyThread, NotBurnCPU) {
  NotBurnCPU(_tps[0], 1);
}
TEST_F(MultiLightThread, NotBurnCPU) {
  NotBurnCPU(_tps[0], kCoresCount);
}
TEST_F(MultiHeavyThread, NotBurnCPU) {
  NotBurnCPU(_tps[0], kCoresCount);
}

#endif

}  // namespace
}  // namespace test

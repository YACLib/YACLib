#include "util/cpu_time.hpp"
#include "util/time.hpp"

#include <yaclib/executor/thread_pool.hpp>

#include <atomic>
#include <iostream>
#include <thread>

#include <gtest/gtest.h>

namespace {

const auto kCoresCount = [] {
  auto const cores_count{std::thread::hardware_concurrency()};
  EXPECT_GT(cores_count, 1);
  return cores_count;
}();

using namespace yaclib;
using namespace std::chrono_literals;

enum class FactoryTag {
  Light = 0,
  Heavy,
};

enum class ThreadPoolTag {
  Single = 0,
  Multi,
};

class SingleLightThread : public ::testing::Test {
 protected:
  void SetUp() override {
    _factory = executor::MakeThreadFactory();
    PushTP();
  }

  void TearDown() override {
    _tps.clear();
  }

  void PushTP() {
    _tps.push_back(executor::MakeThreadPool(1, _factory));
  }

  void PopTP() {
    _tps.pop_back();
  }

  std::vector<executor::IThreadPoolPtr> _tps;
  executor::IThreadFactoryPtr _factory;
  FactoryTag _factory_tag{FactoryTag::Light};
  FactoryTag _tp_tag{ThreadPoolTag::Single};
};

class SingleHeavyThread : public ::testing::Test {
 protected:
  void SetUp() override {
    _factory = executor::MakeThreadFactory(1);
    PushTP();
  }

  void TearDown() override {
    _tps.clear();
  }

  void PushTP() {
    _tps.push_back(executor::MakeThreadPool(1, _factory));
  }

  void PopTP() {
    _tps.pop_back();
  }

  std::vector<executor::IThreadPoolPtr> _tps;
  executor::IThreadFactoryPtr _factory;
  FactoryTag _factory_tag{FactoryTag::Heavy};
  FactoryTag _tp_tag{ThreadPoolTag::Single};
};

class MultiLightThread : public ::testing::Test {
 protected:
  void SetUp() override {
    _factory = executor::MakeThreadFactory();
    PushTP();
  }

  void TearDown() override {
    _tps.clear();
  }

  void PushTP() {
    _tps.push_back(executor::MakeThreadPool(kCoresCount, _factory));
  }

  void PopTP() {
    _tps.pop_back();
  }

  std::vector<executor::IThreadPoolPtr> _tps;
  executor::IThreadFactoryPtr _factory;
  FactoryTag _factory_tag{FactoryTag::Heavy};
  FactoryTag _tp_tag{ThreadPoolTag::Multi};
};

class MultiHeavyThread : public ::testing::Test {
 protected:
  void SetUp() override {
    _factory = executor::MakeThreadFactory(kCoresCount);
    PushTP();
  }

  void TearDown() override {
    _tps.clear();
  }

  void PushTP() {
    _tps.push_back(executor::MakeThreadPool(kCoresCount, _factory));
  }

  void PopTP() {
    _tps.pop_back();
  }

  std::vector<executor::IThreadPoolPtr> _tps;
  executor::IThreadFactoryPtr _factory;
  FactoryTag _factory_tag{FactoryTag::Heavy};
  FactoryTag _tp_tag{ThreadPoolTag::Single};
};

void JustWork(executor::IThreadPoolPtr& tp) {
  bool ready{false};
  tp->Execute([&] {
    ready = true;
  });

  tp->Stop();
  tp->Wait();

  EXPECT_TRUE(ready);
}

TEST_F(SingleLightThread, JustWork) {
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

void ExecuteFrom(executor::IThreadPoolPtr& tp) {
  bool done{false};
  auto task = [&] {
    executor::CurrentThreadPool()->Execute([&] {
      done = true;
    });
  };

  tp->Execute(task);

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

void AfterStopImpl(executor::IThreadPoolPtr& tp, StopType stop_type, bool need_wait) {
  bool ready{false};

  if (need_wait || stop_type != StopType::SoftStop) {
    tp->Execute([&] {
      ready = true;
      if (!need_wait) {
        std::this_thread::sleep_for(1ms);
      }
    });
    std::this_thread::sleep_for(1ms);
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

  tp->Execute([] {
    FAIL();
  });
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

void TwoThreadPool(executor::IThreadPoolPtr& tp1, executor::IThreadPoolPtr& tp2) {
  bool done1{false};
  bool done2{false};

  util::StopWatch stop_watch;

  tp1->Execute([&] {
    tp2->Execute([&] {
      done1 = true;
    });
    std::this_thread::sleep_for(200ms);
  });

  tp2->Execute([&] {
    tp1->Execute([&] {
      done2 = true;
    });
    std::this_thread::sleep_for(200ms);
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

void FIFO(executor::IThreadPoolPtr& tp, StopType stop_type) {
  size_t next_task{0};

  constexpr size_t kTasks{256};
  for (size_t i = 0; i != kTasks; ++i) {
    tp->Execute([i, &next_task] {
      EXPECT_EQ(next_task, i);
      ++next_task;
    });
  }

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

void Exception(executor::IThreadPoolPtr& tp, StopType stop_type) {
  int flag{0};
  tp->Execute([&] {
    flag += 1;
    std::this_thread::sleep_for(1ms);  // Wait stop
    tp->Execute([&] {
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

void ManyTask(executor::IThreadPoolPtr& tp, StopType stop_type, size_t threads_count) {
  const size_t tasks{1024 * threads_count / 3};

  std::atomic_size_t completed{0};
  for (size_t i = 0; i != tasks; ++i) {
    tp->Execute([&completed] {
      completed.fetch_add(1, std::memory_order_relaxed);
    });
  }

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

void UseAllThreads(executor::IThreadPoolPtr& tp, StopType stop_type) {
  std::atomic_size_t counter{0};

  auto sleeper = [&counter] {
    std::this_thread::sleep_for(200ms);
    counter.fetch_add(1, std::memory_order_relaxed);
  };

  util::StopWatch stop_watch;

  tp->Execute(sleeper);
  tp->Execute(sleeper);

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

  auto elapsed = stop_watch.Elapsed();

  EXPECT_EQ(counter, 2);
  EXPECT_GE(elapsed, 200ms);
  EXPECT_LE(elapsed, 350ms);
}

TEST_F(MultiLightThread, UseAllThreads) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = executor::MakeThreadPool(2);
    UseAllThreads(_tps[0], stop_type);
  }
}
TEST_F(MultiHeavyThread, UseAllThreads) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = executor::MakeThreadPool(2);
    UseAllThreads(_tps[0], stop_type);
  }
}

void NotSequentialAndParallel(executor::IThreadPoolPtr& tp, StopType stop_type) {
  // Not sequential start and parallel running
  std::atomic_int counter{0};

  tp->Execute([&] {
    std::this_thread::sleep_for(300ms);
    counter.store(2, std::memory_order_release);
  });

  tp->Execute([&] {
    counter.store(1, std::memory_order_release);
  });

  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(counter.load(std::memory_order_acquire), 1);

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

  EXPECT_EQ(counter.load(std::memory_order_acquire), 2);
}

TEST_F(MultiLightThread, NotSequentialAndParallel) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = executor::MakeThreadPool(2);
    NotSequentialAndParallel(_tps[0], stop_type);
  }
}
TEST_F(MultiHeavyThread, NotSequentialAndParallel) {
  for (auto stop_type : {StopType::SoftStop, StopType::Stop}) {
    _tps[0] = nullptr;  // Release threads
    _tps[0] = executor::MakeThreadPool(2);
    NotSequentialAndParallel(_tps[0], stop_type);
  }
}

void Current(executor::IThreadPoolPtr& tp) {
  EXPECT_EQ(executor::CurrentThreadPool(), nullptr);

  tp->Execute([&] {
    EXPECT_EQ(executor::CurrentThreadPool(), tp);
    std::this_thread::sleep_for(10ms);
    EXPECT_EQ(executor::CurrentThreadPool(), tp);
  });

  EXPECT_EQ(executor::CurrentThreadPool(), nullptr);

  std::this_thread::sleep_for(1ms);

  EXPECT_EQ(executor::CurrentThreadPool(), nullptr);

  tp->Stop();

  EXPECT_EQ(executor::CurrentThreadPool(), nullptr);

  tp->Wait();

  EXPECT_EQ(executor::CurrentThreadPool(), nullptr);
}

TEST_F(SingleLightThread, Current) {
  Current(_tps[0]);
}
TEST_F(SingleHeavyThread, Current) {
  Current(_tps[0]);
}
TEST_F(MultiLightThread, Current) {
  Current(_tps[0]);
}
TEST_F(MultiHeavyThread, Current) {
  Current(_tps[0]);
}

void Lifetime(executor::IThreadPoolPtr& tp, size_t threads) {
  class Task final {
   public:
    Task(Task&&) = default;
    Task(const Task&) = delete;
    Task& operator=(Task&&) = delete;
    Task& operator=(const Task&) = delete;

    explicit Task(std::atomic<int>& counter) : _counter{counter} {
    }

    ~Task() {
      if (_done) {
        _counter.fetch_add(1, std::memory_order_release);
      }
    }

    void operator()() {
      std::this_thread::sleep_for(100ms);
      _done = true;
    }

   private:
    bool _done{false};
    std::atomic_int& _counter;
  };

  std::atomic_int dead{0};

  for (size_t i = 0; i != threads; ++i) {
    tp->Execute(Task(dead));
  }

  std::this_thread::sleep_for(100ms * threads / 2);
  EXPECT_EQ(dead.load(std::memory_order_acquire), threads);

  tp->Stop();
  tp->Wait();

  EXPECT_EQ(dead.load(), threads);
}

TEST_F(MultiLightThread, Lifetime) {
  _tps[0] = nullptr;  // Release threads
  _tps[0] = executor::MakeThreadPool(4);
  Lifetime(_tps[0], 4);
}
TEST_F(MultiHeavyThread, Lifetime) {
  _tps[0] = nullptr;  // Release threads
  _tps[0] = executor::MakeThreadPool(4);
  Lifetime(_tps[0], 4);
}

void RacyCounter(executor::IThreadFactoryPtr& factory) {
  auto tp = executor::MakeThreadPool(2 * kCoresCount, factory);

  std::atomic<size_t> counter1{0};
  std::atomic<size_t> counter2{0};

  static const size_t kIncrements = 123456;
  for (size_t i = 0; i < kIncrements; ++i) {
    tp->Execute([&] {
      auto old = counter1.load(std::memory_order_relaxed);
      counter2.fetch_add(1, std::memory_order_relaxed);
      std::this_thread::yield();
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

void NotBurnCPU(executor::IThreadPoolPtr& tp, size_t threads) {
  // Warmup
  for (size_t i = 0; i != threads; ++i) {
    tp->Execute([&] {
      std::this_thread::sleep_for(100ms);
    });
  }

  util::ProcessCPUTimer cpu_timer;

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

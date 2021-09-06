#include <util/time.hpp>

#include <yaclib/executor/serial.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <atomic>
#include <stack>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace {
using namespace yaclib;
using namespace std::chrono_literals;

// TODO(kononovk): add expect threads, current_executes

TEST(execute_task, simple) {
  auto tp = executor::MakeThreadPool(4);
  auto strand = executor::MakeSerial(tp);
  EXPECT_EQ(strand->Tag(), executor::IExecutor::Type::Serial);

  bool done{false};

  strand->Execute([&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();

  EXPECT_TRUE(done);
}

TEST(counter, simple) {
  auto tp = executor::MakeThreadPool(13);
  auto strand = executor::MakeSerial(tp);

  size_t counter = 0;
  static const size_t kIncrements = 65536;

  for (size_t i = 0; i < kIncrements; ++i) {
    strand->Execute([&counter] {
      ++counter;
    });
  };

  tp->SoftStop();
  tp->Wait();

  EXPECT_EQ(counter, kIncrements);
}

TEST(fifo, simple) {
  auto tp = executor::MakeThreadPool(13);
  auto strand = executor::MakeSerial(tp);

  size_t next_ticket = 0;
  static const size_t kTickets = 123456;

  for (size_t t = 0; t < kTickets; ++t) {
    strand->Execute([&next_ticket, t] {
      EXPECT_EQ(next_ticket, t);
      ++next_ticket;
    });
  }

  tp->SoftStop();
  tp->Wait();

  EXPECT_EQ(next_ticket, kTickets);
}

class Counter {
 public:
  Counter(executor::IExecutorPtr e) : strand_{executor::MakeSerial(e)} {
  }

  void Increment() {
    strand_->Execute([this] {
      ++value_;
    });
  }

  size_t Value() const {
    return value_;
  }

 private:
  size_t value_{0};
  executor::IExecutorPtr strand_;
};

TEST(concurrent_strands, simple) {
  auto tp = executor::MakeThreadPool(16);

  static const size_t kStrands = 50;

  std::vector<Counter> counters;
  counters.reserve(kStrands);
  for (size_t i = 0; i < kStrands; ++i) {
    counters.emplace_back(tp);
  }

  static const size_t kBatchSize = 25;
  static const size_t kIterations = 25;
  for (size_t i = 0; i < kIterations; ++i) {
    for (size_t j = 0; j < kStrands; ++j) {
      for (size_t k = 0; k < kBatchSize; ++k) {
        counters[j].Increment();
      }
    }
  }

  tp->SoftStop();
  tp->Wait();

  for (size_t i = 0; i < kStrands; ++i) {
    EXPECT_EQ(counters[i].Value(), kBatchSize * kIterations);
  }
}

TEST(batching, simple) {
  auto tp = executor::MakeThreadPool(1);
  EXPECT_EQ(tp->Tag(), executor::IExecutor::Type::SingleThread);
  tp->Execute([] {
    // bubble
    std::this_thread::sleep_for(1s);
  });

  auto strand = executor::MakeSerial(tp);

  static const size_t kStrandTasks = 100;

  size_t completed = 0;
  for (size_t i = 0; i < kStrandTasks; ++i) {
    strand->Execute([&completed] {
      ++completed;
    });
  };

  tp->SoftStop();
  tp->Wait();

  EXPECT_EQ(completed, kStrandTasks);
}

TEST(strand_over_strand, simple) {
  auto tp = executor::MakeThreadPool(4);

  auto strand = executor::MakeSerial(executor::MakeSerial(executor::MakeSerial(tp)));

  bool done = false;
  strand->Execute([&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();

  EXPECT_TRUE(done);
}

// TODO(kononovk)
// class LifoManualExecutor final : public executor::IExecutor {
// public:
//  void Execute(yaclib::ITaskPtr task) final {
//    tasks_.push(std::move(task));
//  }
//
//  void Drain() {
//    while (!tasks_.empty()) {
//      ITaskPtr next = std::move(tasks_.top());
//      tasks_.pop();
//      next->Call();
//    }
//  }
//
// private:
//  std::stack<yaclib::ITaskPtr> tasks_;
//};
//
// TEST(stack, cimple) {
//  auto lifo = std::make_shared<LifoManualExecutor>();
//  auto strand = executor::MakeSerial(lifo);
//
//  int steps = 0;
//
//  strand->Execute([&steps] {
//    EXPECT_EQ(steps, 0);
//    ++steps;
//  });
//  strand->Execute([&steps] {
//    EXPECT_EQ(steps, 1);
//    ++steps;
//  });
//
//  lifo->Drain();
//
//  EXPECT_EQ(steps, 2);
//}

TEST(keep_strong_ref, simple) {
  auto tp = executor::MakeThreadPool(1);

  tp->Execute([] {
    // bubble
    std::this_thread::sleep_for(1s);
  });

  bool done = false;
  executor::MakeSerial(tp)->Execute([&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();
  EXPECT_TRUE(done);
}

TEST(do_not_occupy_thread, simple) {
  auto tp = executor::MakeThreadPool(1);

  auto strand = executor::MakeSerial(tp);

  tp->Execute([] {
    // bubble
    std::this_thread::sleep_for(1s);
  });

  std::atomic<bool> stop{false};

  static const auto kStepPause = 10ms;

  auto step = [] {
    std::this_thread::sleep_for(kStepPause);
  };

  for (size_t i = 0; i < 100; ++i) {
    strand->Execute(step);
  }

  tp->Execute([&stop] {
    stop.store(true);
  });

  while (!stop.load()) {
    strand->Execute(step);
    std::this_thread::sleep_for(kStepPause);
  }

  tp->HardStop();
  tp->Wait();
}

TEST(exceptions, simple) {
  auto tp = executor::MakeThreadPool(1);
  auto strand = executor::MakeSerial(tp);

  tp->Execute([] {
    std::this_thread::sleep_for(1s);
  });

  bool done = false;

  strand->Execute([] {
    throw std::runtime_error("You shall not pass!");
  });
  strand->Execute([&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();
  EXPECT_TRUE(done);
}

TEST(non_blocking_execute, simple) {
  auto tp = executor::MakeThreadPool(1);
  auto strand = executor::MakeSerial(tp);

  strand->Execute([] {
    std::this_thread::sleep_for(2s);
  });

  std::this_thread::sleep_for(500ms);

  util::StopWatch stop_watch;
  strand->Execute([] {
  });
  EXPECT_LE(stop_watch.Elapsed(), 100ms);

  tp->SoftStop();
  tp->Wait();
}

TEST(do_not_block_thread_pool, simple) {
  auto tp = executor::MakeThreadPool(2);
  auto strand = executor::MakeSerial(tp);

  strand->Execute([] {
    std::this_thread::sleep_for(1s);
  });

  std::this_thread::sleep_for(100ms);

  strand->Execute([] {
    std::this_thread::sleep_for(1s);
  });

  std::atomic<bool> done{false};

  tp->Execute([&done] {
    done.store(true);
  });

  std::this_thread::sleep_for(200ms);
  EXPECT_TRUE(done.load());

  tp->HardStop();
  tp->Wait();
}

TEST(memory_leak, simple) {
  auto tp = executor::MakeThreadPool(1);
  auto strand = executor::MakeSerial(tp);

  tp->Execute([] {
    std::this_thread::sleep_for(1s);
  });
  strand->Execute([] {
    // No-op
  });

  tp->HardStop();
  tp->Wait();
}

}  // namespace

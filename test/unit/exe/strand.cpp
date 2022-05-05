#include <util/time.hpp>

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <chrono>
#include <cstddef>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

// TODO(kononovk): add expect threads, current_executes

TEST(ExecuteTask, Simple) {
  auto tp = yaclib::MakeThreadPool(4);
  auto strand = MakeStrand(tp);
  EXPECT_EQ(strand->Tag(), yaclib::IExecutor::Type::Strand);

  bool done{false};

  Submit(*strand, [&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();

  EXPECT_TRUE(done);
}

TEST(counter, simple) {
  auto tp = yaclib::MakeThreadPool(13);
  auto strand = MakeStrand(tp);

  std::size_t counter = 0;
  static const std::size_t kIncrements = 65536;

  for (std::size_t i = 0; i < kIncrements; ++i) {
    Submit(*strand, [&counter] {
      ++counter;
    });
  }

  tp->SoftStop();
  tp->Wait();

  EXPECT_EQ(counter, kIncrements);
}

TEST(fifo, simple) {
  auto tp = yaclib::MakeThreadPool(13);
  auto strand = MakeStrand(tp);

  std::size_t next_ticket = 0;
  static const std::size_t kTickets = 123456;

  for (std::size_t t = 0; t < kTickets; ++t) {
    Submit(*strand, [&next_ticket, t] {
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
  Counter(yaclib::IExecutorPtr e) : _strand{MakeStrand(e)} {
  }

  void Increment() {
    Submit(*_strand, [&] {
      ++_value;
    });
  }

  std::size_t Value() const {
    return _value;
  }

 private:
  std::size_t _value = 0;
  yaclib::IExecutorPtr _strand;
};

TEST(concurrent_strands, simple) {
  auto tp = yaclib::MakeThreadPool(16);

  static const std::size_t kStrands = 50;

  std::vector<Counter> counters;
  counters.reserve(kStrands);
  for (std::size_t i = 0; i < kStrands; ++i) {
    counters.emplace_back(tp);
  }

  static const std::size_t kBatchSize = 25;
  static const std::size_t kIterations = 25;
  for (std::size_t i = 0; i < kIterations; ++i) {
    for (std::size_t j = 0; j < kStrands; ++j) {
      for (std::size_t k = 0; k < kBatchSize; ++k) {
        counters[j].Increment();
      }
    }
  }

  tp->SoftStop();
  tp->Wait();

  for (std::size_t i = 0; i < kStrands; ++i) {
    EXPECT_EQ(counters[i].Value(), kBatchSize * kIterations);
  }
}

TEST(batching, simple) {
  auto tp = yaclib::MakeThreadPool(1);
  EXPECT_EQ(tp->Tag(), yaclib::IExecutor::Type::ThreadPool);
  Submit(*tp, [] {
    // bubble
    yaclib_std::this_thread::sleep_for(1s);
  });

  auto strand = MakeStrand(tp);

  static const std::size_t kStrandTasks = 100;

  std::size_t completed = 0;
  for (std::size_t i = 0; i < kStrandTasks; ++i) {
    Submit(*strand, [&completed] {
      ++completed;
    });
  }

  tp->SoftStop();
  tp->Wait();

  EXPECT_EQ(completed, kStrandTasks);
}

TEST(strand_over_strand, simple) {
  auto tp = yaclib::MakeThreadPool(4);

  auto strand = MakeStrand(MakeStrand(MakeStrand(tp)));

  bool done = false;
  Submit(*strand, [&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();

  EXPECT_TRUE(done);
}

// TODO(kononovk)
// class LifoManualExecutor final : public IExecutor {
// public:
//  void Submit(yaclib::JobPtr task) final {
//    tasks_.push(std::move(task));
//  }
//
//  void Drain() {
//    while (!tasks_.empty()) {
//      JobPtr next = std::move(tasks_.top());
//      tasks_.pop();
//      next->Call();
//    }
//  }
//
// private:
//  std::stack<yaclib::JobPtr> tasks_;
//};
//
// TEST(stack, cimple) {
//  auto lifo = std::make_shared<LifoManualExecutor>();
//  auto strand = MakeStrand(lifo);
//
//  int steps = 0;
//
//  Submit(*strand, [&steps] {
//    EXPECT_EQ(steps, 0);
//    ++steps;
//  });
//  Submit(*strand, [&steps] {
//    EXPECT_EQ(steps, 1);
//    ++steps;
//  });
//
//  lifo->Drain();
//
//  EXPECT_EQ(steps, 2);
//}

TEST(keep_strong_ref, simple) {
  auto tp = yaclib::MakeThreadPool(1);

  Submit(*tp, [] {
    // bubble
    yaclib_std::this_thread::sleep_for(1s);
  });

  bool done = false;
  auto strand = MakeStrand(tp);
  Submit(*strand, [&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();
  EXPECT_TRUE(done);
}

TEST(do_not_occupy_thread, simple) {
  auto tp = yaclib::MakeThreadPool(1);

  auto strand = MakeStrand(tp);

  Submit(*tp, [] {
    // bubble
    yaclib_std::this_thread::sleep_for(1s);
  });

  yaclib_std::atomic<bool> stop{false};

  static const auto kStepPause = 10ms;

  auto step = [] {
    yaclib_std::this_thread::sleep_for(kStepPause);
  };

  for (std::size_t i = 0; i < 100; ++i) {
    Submit(*strand, step);
  }

  Submit(*tp, [&stop] {
    stop.store(true);
  });

  while (!stop.load()) {
    Submit(*strand, step);
    yaclib_std::this_thread::sleep_for(kStepPause);
  }

  tp->HardStop();
  tp->Wait();
}

TEST(exceptions, simple) {
  auto tp = yaclib::MakeThreadPool(1);
  auto strand = MakeStrand(tp);

  Submit(*tp, [] {
    yaclib_std::this_thread::sleep_for(1s);
  });

  bool done = false;

  Submit(*strand, [] {
    throw std::runtime_error("You shall not pass!");
  });
  Submit(*strand, [&done] {
    done = true;
  });

  tp->SoftStop();
  tp->Wait();
  EXPECT_TRUE(done);
}

TEST(non_blocking_execute, simple) {
  auto tp = yaclib::MakeThreadPool(1);
  auto strand = MakeStrand(tp);

  Submit(*strand, [] {
    yaclib_std::this_thread::sleep_for(2s);
  });

  yaclib_std::this_thread::sleep_for(500ms);

  test::util::StopWatch stop_watch;
  Submit(*strand, [] {
  });
  EXPECT_LE(stop_watch.Elapsed(), 100ms);

  tp->SoftStop();
  tp->Wait();
}

TEST(do_not_block_thread_pool, simple) {
  auto tp = yaclib::MakeThreadPool(2);
  auto strand = MakeStrand(tp);

  Submit(*strand, [] {
    yaclib_std::this_thread::sleep_for(100ms * YACLIB_CI_SLOWDOWN);
  });

  yaclib_std::this_thread::sleep_for(10ms * YACLIB_CI_SLOWDOWN);

  Submit(*strand, [] {
    yaclib_std::this_thread::sleep_for(100ms * YACLIB_CI_SLOWDOWN);
  });

  yaclib_std::atomic<bool> done = false;

  Submit(*tp, [&done] {
    done.store(true);
  });

  yaclib_std::this_thread::sleep_for(20ms * YACLIB_CI_SLOWDOWN);
  EXPECT_TRUE(done.load());

  tp->HardStop();
  tp->Wait();
}

TEST(memory_leak, simple) {
  auto tp = yaclib::MakeThreadPool(1);
  auto strand = MakeStrand(tp);

  Submit(*tp, [] {
    yaclib_std::this_thread::sleep_for(1s);
  });
  Submit(*strand, [] {
  });

  tp->HardStop();
  tp->Wait();
}

}  // namespace

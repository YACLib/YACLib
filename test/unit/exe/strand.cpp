#include <util/time.hpp>

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/strand.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <chrono>
#include <cstddef>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>
#include <yaclib_std/atomic>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

// TODO(kononovk) add expect threads, current_executes

TEST(ExecuteTask, Simple) {
  yaclib::FairThreadPool tp{4};
  auto strand = MakeStrand(&tp);
  EXPECT_EQ(strand->Tag(), yaclib::IExecutor::Type::Strand);

  bool done{false};

  Submit(*strand, [&done] {
    done = true;
  });

  tp.SoftStop();
  tp.Wait();

  EXPECT_TRUE(done);
}

TEST(Counter, Simple) {
  yaclib::FairThreadPool tp{13};
  auto strand = MakeStrand(&tp);

  std::size_t counter = 0;
  static const std::size_t kIncrements = 65536;

  for (std::size_t i = 0; i < kIncrements; ++i) {
    Submit(*strand, [&counter] {
      ++counter;
    });
  }

  tp.SoftStop();
  tp.Wait();

  EXPECT_EQ(counter, kIncrements);
}

TEST(FIFO, Simple) {
  yaclib::FairThreadPool tp{13};
  auto strand = MakeStrand(&tp);

  std::size_t next_ticket = 0;
  static const std::size_t kTickets = 123456;

  for (std::size_t t = 0; t < kTickets; ++t) {
    Submit(*strand, [&next_ticket, t] {
      EXPECT_EQ(next_ticket, t);
      ++next_ticket;
    });
  }

  tp.SoftStop();
  tp.Wait();

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

TEST(ConcurrentStrands, Simple) {
  yaclib::FairThreadPool tp{16};

  static const std::size_t kStrands = 50;

  std::vector<Counter> counters;
  counters.reserve(kStrands);
  for (std::size_t i = 0; i < kStrands; ++i) {
    counters.emplace_back(&tp);
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

  tp.SoftStop();
  tp.Wait();

  for (std::size_t i = 0; i < kStrands; ++i) {
    EXPECT_EQ(counters[i].Value(), kBatchSize * kIterations);
  }
}

TEST(Batching, Simple) {
  yaclib::FairThreadPool tp{1};
  EXPECT_EQ(tp.Tag(), yaclib::IExecutor::Type::FairThreadPool);
  Submit(tp, [] {
    // bubble
    yaclib_std::this_thread::sleep_for(1s);
  });

  auto strand = MakeStrand(&tp);

  static const std::size_t kStrandTasks = 100;

  std::size_t completed = 0;
  for (std::size_t i = 0; i < kStrandTasks; ++i) {
    Submit(*strand, [&completed] {
      ++completed;
    });
  }

  tp.SoftStop();
  tp.Wait();

  EXPECT_EQ(completed, kStrandTasks);
}

TEST(StrandOverStrand, Simple) {
  yaclib::FairThreadPool tp{4};

  auto strand = MakeStrand(MakeStrand(MakeStrand(&tp)));

  bool done = false;
  Submit(*strand, [&done] {
    done = true;
  });

  tp.SoftStop();
  tp.Wait();

  EXPECT_TRUE(done);
}

class LifoManualExecutor final : public yaclib::IExecutor {
 public:
  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Custom;
  }

  [[nodiscard]] bool Alive() const noexcept final {
    return true;
  }

  void Submit(yaclib::Job& job) noexcept final {
    _jobs.PushFront(job);
  }

  void Drain() noexcept {
    while (!_jobs.Empty()) {
      auto& job = static_cast<yaclib::Job&>(_jobs.PopFront());
      job.Call();
    }
  }

 private:
  yaclib::detail::List _jobs;
};

TEST(Stack, Simple) {
  LifoManualExecutor lifo;
  yaclib::Strand strand{&lifo};

  int steps = 0;

  Submit(strand, [&steps] {
    EXPECT_EQ(steps, 0);
    ++steps;
  });
  Submit(strand, [&steps] {
    EXPECT_EQ(steps, 1);
    ++steps;
  });

  lifo.Drain();

  EXPECT_EQ(steps, 2);
}

TEST(KeepStrongRef, Simple) {
  yaclib::FairThreadPool tp{1};

  Submit(tp, [] {
    // bubble
    yaclib_std::this_thread::sleep_for(1s);
  });

  bool done = false;
  auto strand = MakeStrand(&tp);
  Submit(*strand, [&done] {
    done = true;
  });

  tp.SoftStop();
  tp.Wait();
  EXPECT_TRUE(done);
}

TEST(DoNotOccupyThread, Simple) {
  yaclib::FairThreadPool tp{1};

  auto strand = MakeStrand(&tp);

  Submit(tp, [] {
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

  Submit(tp, [&stop] {
    stop.store(true);
  });

  while (!stop.load()) {
    Submit(*strand, step);
    yaclib_std::this_thread::sleep_for(kStepPause);
  }

  tp.HardStop();
  tp.Wait();
}

TEST(Exceptions, Simple) {
  yaclib::FairThreadPool tp{1};
  auto strand = MakeStrand(&tp);

  Submit(tp, [] {
    yaclib_std::this_thread::sleep_for(1s);
  });

  bool done = false;

  Submit(*strand, [] {
    throw std::runtime_error("You shall not pass!");
  });
  Submit(*strand, [&done] {
    done = true;
  });

  tp.SoftStop();
  tp.Wait();
  EXPECT_TRUE(done);
}

TEST(NonBlockingExecute, Simple) {
  yaclib::FairThreadPool tp{1};
  auto strand = MakeStrand(&tp);

  Submit(*strand, [] {
    yaclib_std::this_thread::sleep_for(2s);
  });

  yaclib_std::this_thread::sleep_for(500ms);

  test::util::StopWatch stop_watch;
  Submit(*strand, [] {
  });
  EXPECT_LE(stop_watch.Elapsed(), 100ms);

  tp.SoftStop();
  tp.Wait();
}

TEST(DoNotBlockThreadPool, Simple) {
  yaclib::FairThreadPool tp{2};
  auto strand = MakeStrand(&tp);

  Submit(*strand, [] {
    yaclib_std::this_thread::sleep_for(100ms * YACLIB_CI_SLOWDOWN);
  });

  yaclib_std::this_thread::sleep_for(10ms * YACLIB_CI_SLOWDOWN);

  Submit(*strand, [] {
    yaclib_std::this_thread::sleep_for(100ms * YACLIB_CI_SLOWDOWN);
  });

  yaclib_std::atomic<bool> done = false;

  Submit(tp, [&done] {
    done.store(true);
  });

  yaclib_std::this_thread::sleep_for(20ms * YACLIB_CI_SLOWDOWN);
  EXPECT_TRUE(done.load());

  tp.HardStop();
  tp.Wait();
}

TEST(MemoryLeak, Simple) {
  yaclib::FairThreadPool tp{1};
  auto strand = MakeStrand(&tp);
  EXPECT_TRUE(strand->Alive());

  Submit(tp, [] {
    yaclib_std::this_thread::sleep_for(1s);
  });
  Submit(*strand, [] {
  });

  tp.HardStop();
  EXPECT_FALSE(strand->Alive());
  tp.Wait();
}

}  // namespace

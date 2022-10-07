#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/atomic>
#include <yaclib_std/random>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

static const std::uint32_t kNumThreads = yaclib_std::thread::hardware_concurrency();
static constexpr std::uint32_t kNumRounds = 100000;

struct StressTest : testing::Test {
  struct Value {
    inline static yaclib_std::atomic_uint32_t created{0};
    inline static yaclib_std::atomic_uint32_t destroyed{0};

    Value(std::uint32_t value) noexcept : v{value} {
      created.fetch_add(1, std::memory_order_relaxed);
    }

    ~Value() {
      destroyed.fetch_add(1, std::memory_order_relaxed);
    }

    std::uint32_t v;
  };

  void SetUp() override {
    Value::created.store(0, std::memory_order_relaxed);
    Value::destroyed.store(0, std::memory_order_relaxed);
  }

  struct alignas(64) Slot {
    yaclib_std::atomic_uint32_t round{0};
    yaclib::FutureOn<Value> future;
  };

  template <typename C>
  void Run(yaclib::IExecutor& e, C consumer) {
    EXPECT_GT(kNumThreads, 1);
    yaclib_std::atomic_uint32_t producer_idx{0};
    yaclib_std::atomic_uint32_t consumer_idx{0};
    yaclib_std::atomic_uint32_t num_resolved_futures{0};
    std::uint32_t num_slots{64 * kNumThreads};
    std::vector<StressTest::Slot> slots{num_slots};
    std::vector<yaclib_std::thread> threads;
    threads.reserve(kNumThreads);

    // producer
    for (std::uint32_t i = 0; i < kNumThreads / 2; ++i) {
      threads.emplace_back(yaclib_std::thread([&producer_idx, i, num_slots, &slots, &e] {
        std::mt19937 rng(i);
        for (std::uint32_t r = 0; r < kNumRounds; ++r) {
          auto [f, p] = yaclib::MakeContractOn<Value>(e);
          auto idx = producer_idx.fetch_add(1, std::memory_order_acq_rel);
          auto slot_round = (idx / num_slots) * 2;
          idx %= num_slots;
          while (slots[idx].round.load(std::memory_order_acquire) != slot_round) {
            yaclib_std::this_thread::yield();
          }
          ASSERT_FALSE(slots[idx].future.Valid());
          slots[idx].future = std::move(f);
          slots[idx].round.store(slot_round + 1, std::memory_order_release);
          volatile auto work = rng() % 2048;
          for (std::uint32_t x = 0; x < work; ++x) {
            [[maybe_unused]] auto _ = work;
          }
          std::move(p).Set(idx * (slot_round + 1));
        }
      }));
    }

    yaclib::WaitGroup<> wg{1};

    // consumer
    for (std::uint32_t i = 0; i < kNumThreads / 2; ++i) {
      threads.emplace_back(
        yaclib_std::thread([consumer, &wg, &consumer_idx, &slots, i, &num_resolved_futures, num_slots]() {
          std::mt19937 rng(kNumThreads + i);
          for (std::uint32_t r = 0; r < kNumRounds; ++r) {
            auto idx = consumer_idx.fetch_add(1, std::memory_order_acq_rel);
            auto slot_round = (idx / num_slots) * 2 + 1;
            idx %= num_slots;
            EXPECT_TRUE(slot_round % 2 != 0);
            while (slots[idx].round.load(std::memory_order_acquire) != slot_round) {
              yaclib_std::this_thread::yield();
            }
            auto f = std::move(slots[idx].future);
            ASSERT_TRUE(f.Valid());
            slots[idx].round.store(slot_round + 1, std::memory_order_release);
            volatile auto work = rng() % 2048;
            for (unsigned x = 0; x < work; ++x) {
              [[maybe_unused]] auto _ = work;
            }
            consumer(wg, std::move(f), idx, slot_round, num_resolved_futures);
          }
        }));
    }
    for (auto& t : threads) {
      t.join();
    }
    wg.Done();
    wg.Wait();
    EXPECT_EQ(kNumRounds * (kNumThreads / 2), num_resolved_futures.load());
    EXPECT_EQ(kNumRounds * (kNumThreads / 2), Value::created.load());
    EXPECT_EQ(kNumRounds * (kNumThreads / 2), Value::destroyed.load());
  }
};

TEST_F(StressTest, ThenInline) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
#if defined(GTEST_OS_WINDOWS) && YACLIB_FAULT == 1
  GTEST_SKIP();  // Too long
#endif
  Run(yaclib::MakeInline(), [](yaclib::WaitGroup<>&, yaclib::FutureOn<Value> future, std::uint32_t idx,
                               std::uint32_t slot_round, auto& num_resolved_futures) {
    std::move(future)
      .ThenInline([slot_round](StressTest::Value&& x) noexcept {
        return x.v / slot_round;
      })
      .DetachInline([&num_resolved_futures, idx](std::uint32_t x) noexcept {
        EXPECT_EQ(idx, x);
        num_resolved_futures.fetch_add(1, std::memory_order_relaxed);
      });
  });
}

TEST_F(StressTest, Then) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
#if defined(GTEST_OS_WINDOWS) && YACLIB_FAULT == 1
  GTEST_SKIP();  // Too long
#endif
  yaclib::FairThreadPool tp;
  Run(tp, [](yaclib::WaitGroup<>& wg, yaclib::FutureOn<Value> future, std::uint32_t idx, std::uint32_t slot_round,
             auto& num_resolved_futures) {
    auto f = std::move(future)
               .Then([slot_round](StressTest::Value&& x) noexcept {
                 return x.v / slot_round;
               })
               .Then([&num_resolved_futures, idx](std::uint32_t x) noexcept {
                 EXPECT_EQ(idx, x);
                 num_resolved_futures.fetch_add(1, std::memory_order_relaxed);
                 return x;
               });
    wg.Consume(std::move(f));
  });
  tp.Stop();
  tp.Wait();
}

}  // namespace
}  // namespace test

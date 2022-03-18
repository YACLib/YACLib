#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/fault/thread.hpp>

#include <array>
#include <random>
#include <stack>
#include <utility>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

yaclib_std::atomic_size_t state = 0;

yaclib::Future<unsigned int> incr(uint64_t delta) {
  unsigned int old = state.fetch_add(delta, std::memory_order_acq_rel);
  co_return old;
}

TEST(StressCoro, IncAtomicSingleThread) {
  static constexpr unsigned int kTestCases = 100'000;
  static constexpr unsigned int kAwaitedFutures = 20;
  static constexpr unsigned int kMaxDelta = 10;

  unsigned int acum = 0;

  unsigned int control_value = 0;

  for (unsigned int tc = 0; tc < kTestCases; ++tc) {
    std::mt19937 rng(tc);
    auto coro = [&]() -> yaclib::Future<unsigned int> {
      std::array<yaclib::Future<unsigned int>, kAwaitedFutures> futures;
      for (unsigned int i = 0; i < kAwaitedFutures; ++i) {
        unsigned int arg = rng() % kMaxDelta;
        control_value += state.load(std::memory_order_relaxed);
        futures[i] = incr(arg);
      }
      co_await Await(futures.begin(), 2);
      unsigned int loc_accum = 0;
      for (auto&& future : futures) {
        loc_accum += std::move(future).Get().Ok();
      }
      co_return loc_accum;
    };
    control_value -= coro().Get().Ok();
  }
  EXPECT_EQ(control_value, 0);
}

}  // namespace

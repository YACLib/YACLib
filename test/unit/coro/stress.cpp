#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>

#include <array>
#include <stack>
#include <utility>
#include <yaclib_std/random>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

std::uint64_t state = 0;

yaclib::Future<std::uint64_t> incr(std::uint64_t delta) {
  auto old = state;
  state += delta;
  co_return old;
}

TEST(StressCoro, IncAtomicSingleThread) {
#if YACLIB_FAULT != 0
  GTEST_SKIP();  // Too long, also we not interested to run single threaded test under fault injection
#endif
  static constexpr std::uint64_t kTestCases = 100'000;
  static constexpr std::uint64_t kAwaitedFutures = 20;
  static constexpr std::uint64_t kMaxDelta = 10;

  std::uint64_t control_value = 0;
  for (std::uint64_t tc = 0; tc != kTestCases; ++tc) {
    std::mt19937_64 rng(tc);
    auto coro = [&]() -> yaclib::Future<std::uint64_t> {
      std::array<yaclib::Future<std::uint64_t>, kAwaitedFutures> futures;
      for (std::uint64_t i = 0; i != kAwaitedFutures; ++i) {
        std::uint64_t arg = rng() % kMaxDelta;
        control_value += state;
        futures[i] = incr(arg);
      }
      co_await Await(futures.begin(), 2);
      std::uint64_t loc_accum = 0;
      for (auto&& future : futures) {
        loc_accum += std::move(future).Touch().Ok();
      }
      co_return loc_accum;
    };
    control_value -= coro().Get().Ok();
  }
  EXPECT_EQ(control_value, 0);
}

}  // namespace
}  // namespace test

#include "yaclib_std/detail/iterator.hpp"

#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/join.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/wait.hpp>
#include <yaclib/async/wait_for.hpp>
#include <yaclib/async/wait_until.hpp>
#include <yaclib/async/when_all.hpp>
#include <yaclib/async/when_any.hpp>

#include <yaclib_std/iterator>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

using Clock = std::chrono::steady_clock;

std::vector<yaclib::Future<int>> GetFuturesBundle(std::size_t count) {
  std::vector<yaclib::Future<int>> futures;
  futures.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    futures.push_back(yaclib::MakeFuture<int>(i));
  }
  return futures;
}

TEST(Join, Variable1) {
  auto f1 = yaclib::MakeFuture(1);
  auto f = yaclib::Join(std::move(f1));
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, Variable2) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f = yaclib::Join(std::move(f1), std::move(f2));
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, Variable3) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);
  auto f = yaclib::Join(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, BeginCount) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::Join(futures.begin(), futures.size());
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, BeginEnd) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::Join(futures.begin(), futures.end());
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, BeginSentinel) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::Join(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, Range) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::Join(futures);
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Wait, Variable1) {
  auto f1 = yaclib::MakeFuture(1);
  yaclib::Wait(f1);
}

TEST(Wait, Variable2) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  yaclib::Wait(f1, f2);
}

TEST(Wait, Variable3) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);
  yaclib::Wait(f1, f2, f3);
}

TEST(Wait, BeginCount) {
  auto futures = GetFuturesBundle(3);
  yaclib::Wait(futures.begin(), futures.size());
}

TEST(Wait, BeginEnd) {
  auto futures = GetFuturesBundle(3);
  yaclib::Wait(futures.begin(), futures.end());
}

TEST(Wait, BeginSentinel) {
  auto futures = GetFuturesBundle(3);
  yaclib::Wait(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
}

TEST(Wait, Range) {
  auto futures = GetFuturesBundle(3);
  yaclib::Wait(futures);
}

TEST(WaitFor, Variable1) {
  auto f1 = yaclib::MakeFuture(1);
  yaclib::WaitFor(1s, f1);
}

TEST(WaitFor, Variable2) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  yaclib::WaitFor(1s, f1, f2);
}

TEST(WaitFor, Variable3) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);
  yaclib::WaitFor(1s, f1, f2, f3);
}

TEST(WaitFor, BeginCount) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitFor(1s, futures.begin(), futures.size());
}

TEST(WaitFor, BeginEnd) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitFor(1s, futures.begin(), futures.end());
}

TEST(WaitFor, BeginSentinel) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitFor(1s, yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
}

TEST(WaitFor, Range) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitFor(1s, futures);
}

TEST(WaitUntil, Variable1) {
  auto f1 = yaclib::MakeFuture(1);
  yaclib::WaitUntil(Clock::now() + 1s, f1);
}

TEST(WaitUntil, Variable2) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  yaclib::WaitUntil(Clock::now() + 1s, f1, f2);
}

TEST(WaitUntil, Variable3) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);
  yaclib::WaitUntil(Clock::now() + 1s, f1, f2, f3);
}

TEST(WaitUntil, BeginCount) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitUntil(Clock::now() + 1s, futures.begin(), futures.size());
}

TEST(WaitUntil, BeginEnd) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitUntil(Clock::now() + 1s, futures.begin(), futures.end());
}

TEST(WaitUntil, BeginSentinel) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitUntil(Clock::now() + 1s, yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
}

TEST(WaitUntil, Range) {
  auto futures = GetFuturesBundle(3);
  yaclib::WaitUntil(Clock::now() + 1s, futures);
}

TEST(WhenAny, Variable1) {
  auto f1 = yaclib::MakeFuture(1);
  auto f = yaclib::WhenAny(std::move(f1));
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAny, Variable2) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f = yaclib::WhenAny(std::move(f1), std::move(f2));
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAny, Variable3) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);
  auto f = yaclib::WhenAny(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAny, BeginCount) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAny(futures.begin(), futures.size());
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAny, BeginEnd) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAny(futures.begin(), futures.end());
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAny, BeginSentinel) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAny(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAny, Range) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAny(futures);
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, Variable1) {
  auto f1 = yaclib::MakeFuture(1);
  auto f = yaclib::WhenAll(std::move(f1));
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, Variable2) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f = yaclib::WhenAll(std::move(f1), std::move(f2));
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, Variable3) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);
  auto f = yaclib::WhenAll(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, BeginCount) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAll(futures.begin(), futures.size());
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, BeginEnd) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAll(futures.begin(), futures.end());
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, BeginSentinel) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAll(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WhenAll, Range) {
  auto futures = GetFuturesBundle(3);
  auto f = yaclib::WhenAll(futures);
  EXPECT_TRUE(std::move(f).Get());
}

TEST(WaitGroup, Variable1) {
  {
    yaclib::WaitGroup<> wg{1};
    auto f1 = yaclib::MakeFuture(1);
    wg.Attach(f1);
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto f1 = yaclib::MakeFuture(1);
    wg.Consume(std::move(f1));
    wg.Done();
    wg.Wait();
  }
}

TEST(WaitGroup, Variable2) {
  {
    yaclib::WaitGroup<> wg{1};
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    wg.Attach(f1, f2);
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    wg.Consume(std::move(f1), std::move(f2));
    wg.Done();
    wg.Wait();
  }
}

TEST(WaitGroup, Variable3) {
  {
    yaclib::WaitGroup<> wg{1};
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    auto f3 = yaclib::MakeFuture(3);
    wg.Attach(f1, f2, f3);
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    auto f3 = yaclib::MakeFuture(3);
    wg.Consume(std::move(f1), std::move(f2), std::move(f3));
    wg.Done();
    wg.Wait();
  }
}

TEST(WaitGroup, BeginCount) {
  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Attach(futures.begin(), futures.size());
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Consume(futures.begin(), futures.size());
    wg.Done();
    wg.Wait();
  }
}

TEST(WaitGroup, BeginEnd) {
  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Attach(futures.begin(), futures.end());
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Consume(futures.begin(), futures.end());
    wg.Done();
    wg.Wait();
  }
}

TEST(WaitGroup, BeginSentinel) {
  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Attach(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Consume(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
    wg.Done();
    wg.Wait();
  }
}

TEST(WaitGroup, Range) {
  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Attach(futures);
    wg.Done();
    wg.Wait();
  }

  {
    yaclib::WaitGroup<> wg{1};
    auto futures = GetFuturesBundle(3);
    wg.Consume(futures);
    wg.Done();
    wg.Wait();
  }
}

}  // namespace
}  // namespace test
#include <util/async_suite.hpp>
#include <util/time.hpp>
#include <util/when_suite.hpp>

#include <yaclib/async/make.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/await_inline.hpp>
#include <yaclib/coro/await_on.hpp>
#include <yaclib/coro/await_sticky.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <yaclib_std/iterator>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

std::vector<yaclib::Future<int>> GetFuturesBundle(std::size_t count) {
  std::vector<yaclib::Future<int>> futures;
  futures.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    futures.push_back(yaclib::MakeFuture<int>(i));
  }
  return futures;
}

TEST(Await, Variable1) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    co_await Await(f1);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Await, Variable2) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    co_await Await(f1, f2);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Await, Variable3) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    auto f3 = yaclib::MakeFuture(3);
    co_await Await(f1, f2, f3);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Await, BeginCount) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await Await(futures.begin(), futures.size());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Await, BeginEnd) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await Await(futures.begin(), futures.end());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Await, BeginSentinel) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await Await(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Await, Range) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await Await(futures);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, Variable1) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    co_await AwaitInline(f1);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, Variable2) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    co_await AwaitInline(f1, f2);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, Variable3) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    auto f3 = yaclib::MakeFuture(3);
    co_await AwaitInline(f1, f2, f3);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, BeginCount) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitInline(futures.begin(), futures.size());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, BeginEnd) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitInline(futures.begin(), futures.end());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, BeginSentinel) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitInline(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitInline, Range) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitInline(futures);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitOn, Variable1) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    co_await AwaitOn(tp, f1);
    co_return{};
  };

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitOn, Variable2) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    co_await AwaitOn(tp, f1, f2);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitOn, Variable3) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    auto f3 = yaclib::MakeFuture(3);
    co_await AwaitOn(tp, f1, f2, f3);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitOn, BeginCount) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitOn(tp, futures.begin(), futures.size());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitOn, BeginEnd) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitOn(tp, futures.begin(), futures.end());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitOn, BeginSentinel) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitOn(tp, yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitOn, Range) {
  yaclib::FairThreadPool tp{1};

  auto coro = [&]() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitOn(tp, futures);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});

  tp.HardStop();
  tp.Wait();
}

TEST(AwaitSticky, Variable1) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    co_await AwaitSticky(f1);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitSticky, Variable2) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    co_await AwaitSticky(f1, f2);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitSticky, Variable3) {
  auto coro = []() -> yaclib::Future<> {
    auto f1 = yaclib::MakeFuture(1);
    auto f2 = yaclib::MakeFuture(2);
    auto f3 = yaclib::MakeFuture(3);
    co_await AwaitSticky(f1, f2, f3);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitSticky, BeginCount) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitSticky(futures.begin(), futures.size());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitSticky, BeginEnd) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitSticky(futures.begin(), futures.end());
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitSticky, BeginSentinel) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitSticky(yaclib_std::counted_iterator{futures.begin(), 3}, yaclib_std::default_sentinel);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(AwaitSticky, Range) {
  auto coro = []() -> yaclib::Future<> {
    auto futures = GetFuturesBundle(3);
    co_await AwaitSticky(futures);
    co_return{};
  };

  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

}  // namespace
}  // namespace test
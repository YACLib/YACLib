#include <util/async_suite.hpp>
#include <util/time.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/when_all.hpp>
#include <yaclib/async/when_any.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/coro/task.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/lazy/make.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <array>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

TYPED_TEST(AsyncSuite, JustWorksPack) {
  yaclib::FairThreadPool tp;
  auto coro = [&]() -> std::conditional_t<TestFixture::kIsFuture, yaclib::Future<int>, yaclib::Task<int>> {
    auto f1 = yaclib::Run(tp, [] {
      yaclib_std::this_thread::sleep_for(1ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    auto f2 = yaclib::Run(tp, [] {
      return 2;
    });

    co_await Await(f1, f2);
    co_return std::move(f1).Touch().Ok() + std::move(f2).Touch().Ok();
  };
  auto f = coro();
  EXPECT_EQ(std::move(f).Get().Ok(), 3);
  tp.HardStop();
  tp.Wait();
}

TYPED_TEST(AsyncSuite, JustWorksRange) {
  yaclib::FairThreadPool tp;
  auto coro = [&]() -> std::conditional_t<TestFixture::kIsFuture, yaclib::Future<int>, yaclib::Task<int>> {
    std::array<yaclib::FutureOn<int>, 2> arr;
    arr[0] = yaclib::Run(tp, [] {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    arr[1] = yaclib::Run(tp, [] {
      return 2;
    });

    co_await yaclib::Await(arr.begin(), 2);
    // co_return yaclib::StopTag{};
    co_return std::move(arr[0]).Touch().Ok() + std::move(arr[1]).Touch().Ok();
  };
  auto future = coro();
  EXPECT_EQ(std::move(future).Get().Ok(), 3);
  tp.HardStop();
  tp.Wait();
}

TYPED_TEST(AsyncSuite, JustWorksCoAwait) {
  yaclib::FairThreadPool tp;
  auto coro = [&]() -> std::conditional_t<TestFixture::kIsFuture, yaclib::Future<int>, yaclib::Task<int>> {
    auto f1 = yaclib::Run(tp, [] {
      yaclib_std::this_thread::sleep_for(50ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });

    co_return co_await std::move(f1);
  };
  auto future = coro();
  EXPECT_EQ(std::move(future).Get().Ok(), 1);
  tp.HardStop();
  tp.Wait();
}

TYPED_TEST(AsyncSuite, CheckSuspend) {
  int counter = 0;
  yaclib::FairThreadPool tp{2};
  const auto coro_sleep_time = 50ms * YACLIB_CI_SLOWDOWN;
  auto was = yaclib_std::chrono::steady_clock::now();
  std::atomic_bool barrier = false;

  auto coro = [&]() -> typename TestFixture::Type {
    counter = 1;
    auto future1 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });
    auto future2 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });

    co_await Await(future1, future2);

    EXPECT_TRUE(barrier.load(std::memory_order_acquire));
    while (!barrier.load(std::memory_order_acquire)) {
    }

    counter = 2;
    co_return{};
  };

  auto outer_future = coro();
  if constexpr (TestFixture::kIsFuture) {
    EXPECT_EQ(1, counter);
  } else {
    EXPECT_EQ(0, counter);
  }
  barrier.store(true, std::memory_order_release);
  EXPECT_LT(yaclib_std::chrono::steady_clock::now() - was, coro_sleep_time);

  std::ignore = std::move(outer_future).Get();

  EXPECT_EQ(2, counter);
  EXPECT_GE(yaclib_std::chrono::steady_clock::now() - was, coro_sleep_time);
  tp.HardStop();
  tp.Wait();
}

TYPED_TEST(AsyncSuite, AwaitNoSuspend) {
  int counter = 0;

  auto coro = [&]() -> typename TestFixture::Type {
    counter = 1;
    auto future = yaclib::MakeFuture();

    co_await Await(future);
    counter = 2;
    co_return{};
  };

  auto outer_future = coro();
  if constexpr (TestFixture::kIsFuture) {
    EXPECT_EQ(2, counter);
  } else {
    EXPECT_EQ(0, counter);
  }
  std::ignore = std::move(outer_future).Get();
}

TYPED_TEST(AsyncSuite, AwaitSingleNoSuspend) {
  int counter = 0;

  auto coro = [&]() -> typename TestFixture::Type {
    counter = 1;
    auto future = yaclib::MakeFuture();

    co_await std::move(future);
    counter = 2;
    co_return{};
  };

  auto outer_future = coro();
  if constexpr (TestFixture::kIsFuture) {
    EXPECT_EQ(2, counter);
  } else {
    EXPECT_EQ(0, counter);
  }
  std::ignore = std::move(outer_future).Get();
}

TYPED_TEST(AsyncSuite, CheckCallback) {
  int counter = 0;

  auto coro = [&]() -> typename TestFixture::Type {
    ++counter;
    co_return{};
  };
  std::ignore = coro()
                  .ThenInline([&] {
                    ++counter;
                  })
                  .Get()
                  .Ok();
  EXPECT_EQ(counter, 2);
}

TEST(CoroFuture, WhenAll) {
  yaclib::FairThreadPool tp{1};

  yaclib_std::atomic_int counter = 0;
  auto coro = [&]() -> yaclib::Future<int> {
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(10ms);
    co_return counter.fetch_add(1);
  };
  auto f = yaclib::WhenAll(coro(), coro());
  auto results = std::move(f).Get().Ok();
  std::vector<int> expected{0, 1};
  EXPECT_EQ(results, expected);

  tp.HardStop();
  tp.Wait();
}

TEST(CoroFuture, WhenAny) {
  yaclib::FairThreadPool tp{1};

  yaclib_std::atomic_int counter = 0;
  auto coro = [&]() -> yaclib::Future<int> {
    co_await On(tp);
    yaclib_std::this_thread::sleep_for(10ms);
    co_return counter.fetch_add(1);
  };
  auto f = yaclib::WhenAny(coro(), coro());
  auto result = std::move(f).Get().Ok();
  EXPECT_EQ(result, 0);

  tp.HardStop();
  tp.Wait();
}

TEST(CoroTask, Check) {
  auto coro = []() -> yaclib::Task<int> {
    auto task = yaclib::MakeTask(10);
    co_await Await(task);
    co_return std::as_const(task).Touch();
  };
  EXPECT_EQ(coro().Get().Ok(), 10);
}

TEST(Future, WhenAllCoro) {
  yaclib::ManualExecutor e;
  auto coro = [&]() -> yaclib::Future<> {
    co_await On(e);
    co_return{};
  };
  auto f1 = coro();
  auto f2 = coro();
  auto f3 = yaclib::WhenAll<yaclib::FailPolicy::FirstFail, yaclib::OrderPolicy::Same>(std::move(f1), std::move(f2));
  std::ignore = e.Drain();
  EXPECT_EQ(std::move(f3).Get().Ok(), yaclib::Unit{});
}

TEST(Coro, FutureUnwrapping) {
  yaclib::ManualExecutor e;
  auto coro = [&](int x) -> yaclib::Future<int> {
    co_await On(e);
    EXPECT_EQ(x, 1);
    co_return x * 2;
  };
  auto f = yaclib::MakeFuture(1).ThenInline([&](int x) {
    return coro(x)
      .ThenInline([](int y) {
        return y * 2;
      })
      .ThenInline([](int y) {
        return y * 2;
      })
      .ThenInline([](int y) {
        return y * 2;
      });
  });
  std::ignore = e.Drain();
  EXPECT_EQ(std::move(f).Get().Ok(), 16);
  f = yaclib::MakeFuture(1).ThenInline([&](int x) {
    return yaclib::Run(e,
                       [x] {
                         return x;
                       })
      .ThenInline([](int y) {
        return y * 2;
      })
      .ThenInline([](int y) {
        return y * 2;
      })
      .ThenInline([](int y) {
        return y * 2;
      });
  });
  std::ignore = e.Drain();
  EXPECT_EQ(std::move(f).Get().Ok(), 8);
}

TYPED_TEST(AsyncSuite, CoroTaskUnwrapping) {
  auto coro = [](int x) -> yaclib::Task<int> {
    EXPECT_EQ(x, 1);
    co_return x * 2;
  };
  auto f = INVOKE(yaclib::MakeInline(), [] {
             return 1;
           }).ThenInline([&](int x) {
    return coro(x)
      .ThenInline([](int y) {
        return y * 2;
      })
      .ThenInline([](int y) -> yaclib::Task<int> {
        co_return y * 2;
      })
      .ThenInline([](int y) {
        return y * 2;
      });
  });

  EXPECT_EQ(std::move(f).Get().Ok(), 16);
  f = INVOKE(yaclib::MakeInline(), [] {
        return 1;
      }).ThenInline([&](int x) {
    return yaclib::MakeTask(x)
      .ThenInline([](int y) {
        return y * 2;
      })
      .ThenInline([](int y) -> yaclib::Task<int> {
        co_return y * 2;
      })
      .ThenInline([](int y) {
        return y * 2;
      });
  });
  EXPECT_EQ(std::move(f).Get().Ok(), 8);
}

TEST(Coro, WhenAllCoAwait) {
  yaclib::ManualExecutor e;
  auto coro = [&]() -> yaclib::Future<> {
    co_await On(e);
    co_return{};
  };
  auto coro2 = [&]() -> yaclib::Future<int> {
    auto f1 = coro();
    auto f2 = coro();
    auto f3 = yaclib::WhenAny(std::move(f1), std::move(f2));
    co_await Await(f3);
    co_return 42;
  };
  auto f = coro2();
  std::ignore = e.Drain();
  EXPECT_EQ(std::move(f).Get().Ok(), 42);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

yaclib::Task<int> recursiveLazyFunc(int x) {
  return yaclib::MakeTask(x).ThenInline([](int y) -> yaclib::Task<int> {
    if (y <= 0) {
      EXPECT_EQ(y, 0);  // set break point here
      return yaclib::MakeTask(42);
    } else {
      return recursiveLazyFunc(y - 1);
    }
  });
}

yaclib::FutureOn<int> recursiveEagerNotReadyFunc(yaclib::IExecutor& e, int x) {
  return yaclib::Run(e,
                     [x] {
                       return x;
                     })
    .ThenInline([&](int y) -> yaclib::FutureOn<int> {
      if (y <= 0) {
        EXPECT_EQ(y, 0);  // set break point here
        return yaclib::Run(e, [] {
          return 42;
        });
      } else {
        return recursiveEagerNotReadyFunc(e, y - 1);
      }
    });
}

yaclib::Future<int> recursiveEagerFunc(int x) {
  return yaclib::MakeFuture(x).ThenInline([](int y) -> yaclib::Future<int> {
    if (y <= 0) {
      EXPECT_EQ(y, 0);  // set break point here
      return yaclib::MakeFuture(42);
    } else {
      return recursiveEagerFunc(y - 1);
    }
  });
}

yaclib::Future<int> recursiveEagerFuncStupid(int x) {
  auto [f, p] = yaclib::MakeContract<int>();
  yaclib::MakeFuture(x).DetachInline([p = std::move(p)](int y) mutable {
    if (y <= 0) {
      EXPECT_EQ(y, 0);  // set break point here
      std::move(p).Set(42);
    } else {
      recursiveEagerFuncStupid(y - 1).DetachInline([p = std::move(p)](int z) mutable {
        std::move(p).Set(z);
      });
    }
  });
  return std::move(f);
}

yaclib::Task<int> recursiveLazyCoro(int x) {
  if (x <= 0) {
    EXPECT_EQ(x, 0);  // set break point here
    co_return 42;
  }
  auto task = recursiveLazyCoro(x - 1);
  co_await Await(task);
  co_return std::move(task).Touch();  // set break point here, gdb not cool with coro
}

yaclib::Future<int> recursiveEagerCoro(int x) {
  if (x <= 0) {
    EXPECT_EQ(x, 0);  // set break point here
    co_return 42;
  }
  auto task = recursiveEagerCoro(x - 1);
  co_await Await(task);
  co_return std::move(task).Touch();  // set break point here, gdb not cool with coro
}

static constexpr int kLazyRecursion = 1'000'000;
static constexpr int kEagerRecursion = 100;
static constexpr int kBadFactor = 10;

TEST(Recursion, LazyFunc) {
  EXPECT_EQ(recursiveLazyFunc(kLazyRecursion).Get().Ok(), 42);
}

TEST(Recursion, EagerNotReadyFunc) {
  yaclib::ManualExecutor e;
  auto f = recursiveEagerNotReadyFunc(e, kLazyRecursion);
  std::ignore = e.Drain();
  EXPECT_EQ(std::move(f).Get().Ok(), 42);
}

TEST(Recursion, EagerFunc) {
  // honest recursion, in debug produce recursion * 10 (helper functions) frames, in release * 2
  EXPECT_EQ(recursiveEagerFunc(kEagerRecursion / kBadFactor).Get().Ok(), 42);
}

TEST(Recursion, EagerFuncStupid) {
  // honest recursion, in debug produce recursion * 10 (helper functions) frames, in release * 3
  EXPECT_EQ(recursiveEagerFuncStupid(kEagerRecursion / kBadFactor).Get().Ok(), 42);
}

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0 && defined(__clang__) && defined(__linux__)
// GCC Debug bad in symmetric transfer
// I'm also not sure about llvm clang on apple, apple clang, and clangcl
static constexpr int kLazyCoroRecursion = kLazyRecursion;
static constexpr int kEagerCoroRecursion = kEagerRecursion;
#else
static constexpr int kLazyCoroRecursion = kEagerRecursion / kBadFactor;
static constexpr int kEagerCoroRecursion = kEagerRecursion / kBadFactor;
#endif

TEST(Recursion, LazyCoro) {
  EXPECT_EQ(recursiveLazyCoro(kLazyCoroRecursion).Get().Ok(), 42);
}

TEST(Recursion, EagerCoro) {
  // honest recursion, in clang debug/release produce recursion frames
  EXPECT_EQ(recursiveEagerCoro(kEagerCoroRecursion).Get().Ok(), 42);
}

}  // namespace
}  // namespace test

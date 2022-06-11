#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/await_group.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <array>
#include <exception>
#include <stack>
#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

TEST(AwaitGroup, JustWorks) {
#if defined(YACLIB_UBSAN) && (defined(__GLIBCPP__) || defined(__GLIBCXX__))
  GTEST_SKIP();
#endif
  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<int> {
    auto f1 = yaclib::Run(*tp, [] {
      yaclib_std::this_thread::sleep_for(1ms * YACLIB_CI_SLOWDOWN);
      return 1;
    });
    auto f2 = yaclib::Run(*tp, [] {
      return 2;
    });

    yaclib::AwaitGroup await_group;
    await_group.Add(f1);
    await_group.Add(f2);

    co_await await_group;
    co_return std::move(f1).Touch().Ok() + std::move(f2).Touch().Ok();
  };
  auto f = coro(tp);
  EXPECT_EQ(std::move(f).Get().Ok(), 3);
  tp->HardStop();
  tp->Wait();
}

}  // namespace
}  // namespace test

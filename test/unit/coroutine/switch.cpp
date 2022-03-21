#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/coroutine/switch.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <exception>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(Switch, JustWorks) {
  auto main_thread = yaclib_std::this_thread::get_id();

  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<void> {
    co_await yaclib::detail::SwitchAwaiter(std::move(tp));
    auto other_thread = yaclib_std::this_thread::get_id();
    EXPECT_NE(other_thread, main_thread);
    co_return;
  };
  auto f = coro(tp);
  std::ignore = std::move(f).Get();
  tp->HardStop();
  tp->Wait();
}

}  // namespace

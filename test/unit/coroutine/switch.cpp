#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/coroutine/switch.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <array>
#include <exception>
#include <stack>
#include <utility>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

TEST(Switch, JustWorks) {
#if defined(__GNUG__) && (YACLIB_UBSAN == 1 || defined(YACLIB_TSAN))
  GTEST_SKIP();
#endif

  auto main_thread = yaclib_std::this_thread::get_id();

    std::cout << "MAIN THREAD = " << yaclib_std::this_thread::get_id() << '\n';
  

  auto tp = yaclib::MakeThreadPool();
  auto coro = [&](yaclib::IThreadPoolPtr tp) -> yaclib::Future<void> {
    co_await yaclib::detail::SwitchAwaiter(tp);
    
    std::cout << "After co_await switch\n";

    auto other_thread = yaclib_std::this_thread::get_id();

    using namespace std::chrono_literals;

    std::cout << "CURRENT THREAD = " << yaclib_std::this_thread::get_id() << '\n';

    EXPECT_NE(other_thread, main_thread);

    std::cout << "Before co_return\n";
    
    co_return;
  };
  auto f = coro(tp);
  std::move(f).Get();
  tp->HardStop();
  tp->Wait();
}

}  // namespace

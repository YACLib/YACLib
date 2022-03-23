#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_coro_traits.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <array>
#include <exception>
#include <stack>
#include <utility>

#include <gtest/gtest.h>

namespace {
using namespace std::chrono_literals;

/*
 *
 * Without undefined sanitizer:
 * CMake options:
 * -DCMAKE_BUILD_TYPE=Debug
 * -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
 * -GNinja
 * -DYACLIB_CXX_STANDARD=20
 * -DYACLIB_FLAGS="CORO"
 * -DYACLIB_TEST=ON
 * -DYACLIB_LOG="ERROR"
 *
 * YACLIB_LINK_OPTIONS   : -fcoroutines
 * YACLIB_COMPILE_OPTIONS: -fcoroutines
 * libstdc++: 20210728
 *
 * Future(core) %p: 0x7ffd2ead4398; thread: 140377177179968 // coroutine future
 * Future(core) %p: 0x563b47b016a8; thread: 140377177179968 // future1
 * Future(core) %p: 0x563b47b016b0; thread: 140377177179968 // future2
 * future1 %p0x563b47b016a8; future2 %p0x563b47b016b0
 * Get thread: 140377177179968
 * resume thread: 140377168782912
 * future1 %p: 0x563b47b016a8; future2 %p: 0x563b47b016b0
 *
 * ~Future() %p: 0x563b47b016b0; thread: 140377168782912  // future2
 * ~Future() %p: 0x563b47b016a8; thread: 140377168782912  // future1
 * ~Future() %p: 0x7ffd2ead4398; thread: 140377177179968  // coroutine future
 *
 * With undefined sanitizer:
 * CMake options:
 * -DCMAKE_BUILD_TYPE=Debug
 * -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
 * -GNinja
 * -DYACLIB_CXX_STANDARD=20
 * -DYACLIB_FLAGS="CORO;UBSAN"
 * -DYACLIB_TEST=ON
 * -DYACLIB_LOG="ERROR"
 *
 * YACLIB_LINK_OPTIONS   : -fsanitize=undefined;-fcoroutines
 * YACLIB_COMPILE_OPTIONS: -fsanitize=undefined;-fno-omit-frame-pointer;-fcoroutines // same without -fno-omit-frame-pointer
 * libstdc++: 20210728
 * Future(core) %p: 0x7ffc50d7a368; thread: 139835960072064 // coroutine future
 * Future(core) %p: 0x56179d8cf5f8; thread: 139835960072064 // future1
 * Future(core) %p: 0x56179d8cf600; thread: 139835960072064 // future2
 * future1 %p0x56179d8cf5f8; future2 %p0x56179d8cf600
 * Get thread: 139835960072064
 * resume thread: 139835951674944
 * future1 %p: 0x56179d8cf5f8; future2 %p: 0x56179d8cf600
 * ~Future() %p: 0x56179d8ceba0; thread: 139835951674944 // Should be future2. Why do call dtor on this address?
 * ~/projects/YACLib/include/yaclib/async/detail/future_impl.hpp:240:16:
 * runtime error: member call on address 0x7ffc50d7a358 which does not point to an object of type 'BaseCore' 0x7ffc50d7a358:
 * note: object has invalid vptr 17 56 00 00  32 00 00 00 00 00 00 00  15 e6 ab 52 06 04 00 00  00 00 00 00 00 00 00 00
 * f4 68 ab 55
 *               ^~~~~~~~~~~~~~~~~~~~~~~
 *               invalid vptr
 * unit_coroutine_await: /home/mbkkt/projects/YACLib/src/async/base_core.cpp:69: void yaclib::detail::BaseCore::Stop():
 * Assertion `_callback == nullptr' failed. [1]    22494 IOT instruction (core dumped)
 * ./cmake-build-gcc_debug/test/unit_coroutine_await
 * */
TEST(Await, CheckSuspend) {
  int counter = 0;
  auto tp = yaclib::MakeThreadPool(2);
  const auto coro_sleep_time = 50ms * YACLIB_CI_SLOWDOWN;
  auto was = std::chrono::steady_clock::now();
  std::atomic_bool barrier = false;

  auto coro = [&]() -> yaclib::Future<void> {
    counter = 1;
    auto future1 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });
    auto future2 = yaclib::Run(tp, [&] {
      yaclib_std::this_thread::sleep_for(coro_sleep_time);
    });

    std::cerr << "future1 %p: " << &future1 << "; future2 %p: " << &future2 << std::endl;
    co_await Await(future1, future2);
    std::cerr << "future1 %p: " << &future1 << "; future2 %p: " << &future2 << std::endl;

    EXPECT_TRUE(barrier.load(std::memory_order_acquire));
    while (!barrier.load(std::memory_order_acquire)) {
    }

    counter = 2;
    co_return;
  };

  auto outer_future = coro();

  EXPECT_EQ(1, counter);
  barrier.store(true, std::memory_order_release);

  EXPECT_LT(std::chrono::steady_clock::now() - was, coro_sleep_time);

  std::cerr << "Get thread: " << std::this_thread::get_id() << std::endl;
  std::ignore = std::move(outer_future).Get();

  EXPECT_EQ(2, counter);
  EXPECT_GE(std::chrono::steady_clock::now() - was, coro_sleep_time);
  tp->HardStop();
  tp->Wait();
}

}  // namespace

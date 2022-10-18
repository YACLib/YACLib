#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>

#include <exception>
#include <stack>
#include <utility>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

yaclib::Future<int, yaclib::StopError> test_co_ret42() {
  co_return 42;
}

TEST(CoroTraits, CoReturnInt) {
  auto future = test_co_ret42();
  int res = std::move(future).Touch().Ok();
  EXPECT_EQ(res, 42);
}

yaclib::Future<> test_void_coro() {
  co_return{};
}

TEST(CoroTraits, VoidCoro) {
  auto future = test_void_coro();
  EXPECT_TRUE(future.Ready());
}

yaclib::Future<int> double_arg(unsigned x) {
  co_return 2 * x;
}

TEST(CoroTraits, DoubleArg) {
  unsigned value = 42;
  auto future = double_arg(42);
  unsigned result = std::move(future).Touch().Ok();

  EXPECT_EQ(result, 2 * value);
}

yaclib::Future<int> throw_exc_at1(int x) {
  if (x != 1) {
    co_return 42;
  }
  throw std::runtime_error{"From coro"};
}

TEST(CoroTraits, ThrowException) {
  int arg = 1;
  auto future = throw_exc_at1(arg);
  EXPECT_THROW(std::ignore = std::move(future).Touch().Ok(), std::runtime_error);
}

yaclib::Future<> throw_exc_void_at1(int x) {
  if (x == 1) {
    throw std::runtime_error{"From coro"};
  }
  co_return{};
}

TEST(CoroTraits, ThrowExceptionVoid) {
  auto future = throw_exc_void_at1(1);
  EXPECT_THROW(std::ignore = std::move(future).Touch().Ok(), std::runtime_error);
}

yaclib::Future<int> nested_intermed_coro(int x) {
  auto f = test_void_coro();
  EXPECT_TRUE(f.Ready());
  co_return double_arg(x).Touch().Ok();
}

yaclib::Future<int> nested_coro(int x) {
  co_return nested_intermed_coro(x).Touch().Ok();
}

TEST(CoroTraits, NestedCoros) {
  int arg = 42;
  auto future = nested_coro(arg);
  EXPECT_EQ(std::move(future).Touch().Ok(), arg * 2);
}

yaclib::Future<int> coro_ret42() {
  co_return 42;
}

TEST(CoroTraits, CoroWithThen) {
  auto f = coro_ret42();
  int x = std::move(f)
            .ThenInline([](int x) {
              yaclib_std::this_thread::sleep_for(500ms);
              return x + 1;
            })
            .Get()
            .Ok();
  EXPECT_EQ(x, 43);
}

TEST(CoroTraits, CoroWithThen2) {
  int x = coro_ret42()
            .ThenInline([](int x) {
              return x + 1;
            })
            .Get()
            .Ok();
  EXPECT_EQ(x, 43);
}

TEST(CoroTraits, Lambda) {
  auto coro = [&]() -> yaclib::Future<int> {
    co_return 42;
  };

  EXPECT_EQ(coro().Touch().Ok(), 42);
}

}  // namespace
}  // namespace test

#include <util/error_code.hpp>
#include <util/time.hpp>

#include <yaclib/async/make.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/coro/await.hpp>
#include <yaclib/coro/future.hpp>
#include <yaclib/coro/shared_future.hpp>
#include <yaclib/coro/task.hpp>

#include <exception>
#include <stack>
#include <system_error>
#include <utility>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

yaclib::Future<int> test_co_ret42() {
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

TEST(CoroTraits, Shared) {
  auto coro = [&]() -> yaclib::SharedFuture<int> {
    co_return 42;
  };

  auto f1 = coro();
  auto f2 = f1;

  EXPECT_EQ(f2.Touch().Ok(), 42);
}

yaclib::Future<int, ErrorCodeTrait> trait_co_ret42() {
  co_return 42;
}

TEST(CoroTraits, ErrorCodeTraitCoReturnInt) {
  auto future = trait_co_ret42();
  EXPECT_EQ(std::move(future).Get().Ok(), 42);
}

yaclib::Future<int, ErrorCodeTrait> trait_throw_exc_at1(int x) {
  if (x == 1) {
    throw std::runtime_error{"From coro"};
  }
  co_return 42;
}

TEST(CoroTraits, ErrorCodeTraitThrowException) {
  // unhandled_exception stores the exception via ErrorCodeTrait::FromException catch-all
  auto future = trait_throw_exc_at1(1);
  auto result = std::move(future).Get();
  EXPECT_FALSE(ErrorCodeTrait::Ok(result));
  EXPECT_EQ(std::as_const(result).Error(), std::make_error_code(std::errc::io_error));
}

yaclib::Future<int, ErrorCodeTrait> trait_await_failed() {
  auto inner =
    yaclib::MakeFuture<int, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
  co_return co_await std::move(inner);
}

TEST(CoroTraits, ErrorCodeTraitAwaitFailedFuture) {
  // co_await throws std::system_error, FromException maps it back to the original error code
  auto future = trait_await_failed();
  auto result = std::move(future).Get();
  EXPECT_FALSE(ErrorCodeTrait::Ok(result));
  EXPECT_EQ(std::as_const(result).Error(), std::make_error_code(std::errc::invalid_argument));
}

yaclib::Task<int, ErrorCodeTrait> trait_task_co_ret42() {
  co_return 42;
}

TEST(CoroTraits, ErrorCodeTraitTask) {
  auto task = trait_task_co_ret42();
  EXPECT_EQ(std::move(task).Get().Ok(), 42);
}

TEST(CoroTraits, ErrorCodeTraitTaskCancel) {
  auto task = trait_task_co_ret42();
  // Drop stores StopTag -> operation_canceled, the dropped result is unobservable, just must not crash
  std::move(task).Cancel();
}

TEST(CoroTraits, ErrorCodeTraitShared) {
  auto coro = [&]() -> yaclib::SharedFuture<int, ErrorCodeTrait> {
    co_return 42;
  };

  auto f1 = coro();
  auto f2 = f1;

  EXPECT_EQ(f2.Get().Ok(), 42);
}

}  // namespace
}  // namespace test

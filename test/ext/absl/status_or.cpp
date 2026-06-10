#include <yaclib_ext/absl/status_or.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/shared_contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/async/when_all.hpp>
#include <yaclib/config.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/result.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/await.hpp>
#  include <yaclib/coro/future.hpp>
#  include <yaclib/coro/task.hpp>
#endif

#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <gtest/gtest.h>

namespace test {
namespace {

using Trait = yaclib_ext::StatusOrTrait;

// Result containers are raw absl vocabulary types, not yaclib wrappers around them
static_assert(std::is_same_v<Trait::Result<int>, absl::StatusOr<int>>);
static_assert(std::is_same_v<Trait::Result<void>, absl::Status>);
static_assert(std::is_same_v<Trait::Error, absl::Status>);
static_assert(std::is_same_v<Trait::Value<absl::StatusOr<int>>, int>);
static_assert(std::is_same_v<Trait::Value<absl::Status>, void>);

TEST(StatusOrTrait, TraitInt) {
  auto value = Trait::MakeResult<int>(5);
  static_assert(std::is_same_v<decltype(value), absl::StatusOr<int>>);
  EXPECT_TRUE(Trait::Ok(value));
  EXPECT_EQ(Trait::GetValue(std::as_const(value)), 5);
  EXPECT_EQ(Trait::Get(std::move(value)), 5);

  auto unit = Trait::MakeResult<int>(yaclib::Unit{});
  EXPECT_TRUE(Trait::Ok(unit));
  EXPECT_EQ(Trait::GetValue(std::move(unit)), 0);

  auto stop = Trait::MakeResult<int>(yaclib::StopTag{});
  EXPECT_FALSE(Trait::Ok(stop));
  static_assert(std::is_same_v<yaclib::remove_cvref_t<decltype(Trait::GetError(std::as_const(stop)))>, absl::Status>);
  EXPECT_TRUE(Trait::IsStop(Trait::GetError(std::as_const(stop))));
  EXPECT_EQ(Trait::GetError(std::as_const(stop)).code(), absl::StatusCode::kCancelled);

  auto error = Trait::MakeResult<int>(std::make_exception_ptr(std::runtime_error{"boom"}));
  EXPECT_FALSE(Trait::Ok(error));
  EXPECT_FALSE(Trait::IsStop(Trait::GetError(std::as_const(error))));
  EXPECT_EQ(Trait::GetError(std::as_const(error)).code(), absl::StatusCode::kUnknown);
  EXPECT_EQ(Trait::GetError(std::as_const(error)).message(), "boom");
  EXPECT_THROW(std::ignore = Trait::Get(std::move(error)), absl::BadStatusOrAccess);

  // FromException maps cancellation exceptions back to the kCancelled status
  EXPECT_TRUE(Trait::IsStop(Trait::FromException(yaclib::StopPtr())));

  auto head = Trait::MakeResult<int>(absl::NotFoundError("missing"));
  EXPECT_FALSE(Trait::Ok(head));
  EXPECT_EQ(Trait::GetError(std::as_const(head)).code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(Trait::GetError(std::as_const(head)).message(), "missing");

  auto pass = Trait::MakeResult<int>(absl::StatusOr<int>{1});
  EXPECT_TRUE(Trait::Ok(pass));
  EXPECT_EQ(Trait::GetValue(std::move(pass)), 1);

  auto in_place = Trait::MakeResult<int>(std::in_place, 2);
  EXPECT_TRUE(Trait::Ok(in_place));
  EXPECT_EQ(Trait::GetValue(std::move(in_place)), 2);
}

TEST(StatusOrTrait, TraitVoid) {
  auto ok = Trait::MakeResult<void>(yaclib::Unit{});
  static_assert(std::is_same_v<decltype(ok), absl::Status>);
  EXPECT_TRUE(Trait::Ok(ok));
  EXPECT_EQ(ok, absl::OkStatus());
  static_assert(std::is_same_v<decltype(Trait::GetValue(std::as_const(ok))), yaclib::Unit>);
  EXPECT_EQ(Trait::GetValue(std::as_const(ok)), yaclib::Unit{});
  EXPECT_NO_THROW(Trait::Get(std::move(ok)));

  auto stop = Trait::MakeResult<void>(yaclib::StopTag{});
  EXPECT_FALSE(Trait::Ok(stop));
  EXPECT_EQ(stop.code(), absl::StatusCode::kCancelled);
  // GetError of Result<void> is a pass-through: the status is its own error
  static_assert(std::is_same_v<decltype(Trait::GetError(std::as_const(stop))), const absl::Status&>);
  EXPECT_EQ(&Trait::GetError(std::as_const(stop)), &stop);
  EXPECT_TRUE(Trait::IsStop(Trait::GetError(std::as_const(stop))));
  EXPECT_THROW(Trait::Get(std::move(stop)), absl::BadStatusOrAccess);
}

TEST(StatusOrTrait, ContractValue) {
  auto [f, p] = yaclib::MakeContract<int, Trait>();
  EXPECT_FALSE(f.Ready());
  std::move(p).Set(5);
  EXPECT_TRUE(f.Ready());
  auto result = std::move(f).Get();
  // The future result IS an absl::StatusOr<int>, inspect it with the native absl API
  static_assert(std::is_same_v<decltype(result), absl::StatusOr<int>>);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(*result, 5);
  EXPECT_EQ(result.status(), absl::OkStatus());
}

TEST(StatusOrTrait, ContractVoid) {
  auto [f, p] = yaclib::MakeContract<void, Trait>();
  EXPECT_FALSE(f.Ready());
  std::move(p).Set();
  EXPECT_TRUE(f.Ready());
  auto result = std::move(f).Get();
  static_assert(std::is_same_v<decltype(result), absl::Status>);
  EXPECT_TRUE(result.ok());
}

TEST(StatusOrTrait, PromiseDropIsCancelled) {
  auto [f, p] = yaclib::MakeContract<int, Trait>();
  {
    auto dropped = std::move(p);
  }
  EXPECT_TRUE(f.Ready());
  auto result = std::move(f).Get();
  EXPECT_FALSE(result.ok());
  // The headline: dropping the producer surfaces as a kCancelled status in absl's
  // own vocabulary, no exception is thrown anywhere
  EXPECT_EQ(result.status().code(), absl::StatusCode::kCancelled);
  EXPECT_TRUE(Trait::IsStop(result.status()));
}

TEST(StatusOrTrait, RunThenChain) {
  yaclib::FairThreadPool tp;
  auto result = yaclib::Run<Trait>(tp,
                                   [] {
                                     return 40;
                                   })
                  .Then([](int x) {
                    return x + 2;
                  })
                  // Whole-Result callbacks receive the raw absl::StatusOr
                  .Then([](absl::StatusOr<int> x) {
                    return *x;
                  })
                  .Get();
  static_assert(std::is_same_v<decltype(result), absl::StatusOr<int>>);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(*result, 42);
  tp.Stop();
  tp.Wait();
}

TEST(StatusOrTrait, RunThenRecovery) {
  yaclib::FairThreadPool tp;
  auto result = yaclib::Run<Trait>(tp,
                                   [] {
                                     return 1;
                                   })
                  .Then([](int) -> int {
                    throw std::runtime_error{"boom"};
                  })
                  .Then([](int x) {  // Skipped, the chain is in the error state
                    return x + 1;
                  })
                  // The thrown exception arrives at the recovery callback as kUnknown
                  // with the what() message, in absl vocabulary
                  .Then([](absl::Status status) {
                    EXPECT_EQ(status.code(), absl::StatusCode::kUnknown);
                    EXPECT_EQ(status.message(), "boom");
                    return 99;
                  })
                  .Get();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(*result, 99);
  tp.Stop();
  tp.Wait();
}

TEST(StatusOrTrait, MakeFutureError) {
  auto f = yaclib::MakeFuture<int, Trait>(absl::InvalidArgumentError("bad argument"));
  EXPECT_TRUE(f.Ready());
  auto result = std::move(f).Get();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(result.status().message(), "bad argument");
}

TEST(StatusOrTrait, WhenAllFirstFail) {
  auto [f1, p1] = yaclib::MakeContract<int, Trait>();
  auto [f2, p2] = yaclib::MakeContract<int, Trait>();
  auto all = yaclib::WhenAll<yaclib::FailPolicy::FirstFail>(std::move(f1), std::move(f2));
  std::move(p1).Set(absl::InvalidArgumentError("bad argument"));
  std::move(p2).Set(7);
  auto result = std::move(all).Get();
  static_assert(std::is_same_v<decltype(result), absl::StatusOr<std::vector<int>>>);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(result.status().message(), "bad argument");
}

TEST(StatusOrTrait, WhenAllNone) {
  auto [f1, p1] = yaclib::MakeContract<int, Trait>();
  auto [f2, p2] = yaclib::MakeContract<int, Trait>();
  auto all = yaclib::WhenAll<yaclib::FailPolicy::None>(std::move(f1), std::move(f2));
  std::move(p1).Set(7);
  std::move(p2).Set(absl::NotFoundError("missing"));
  auto result = std::move(all).Get();
  static_assert(std::is_same_v<decltype(result), absl::StatusOr<std::vector<absl::StatusOr<int>>>>);
  ASSERT_TRUE(result.ok());
  const auto& items = *result;
  ASSERT_EQ(items.size(), 2);
  ASSERT_TRUE(items[0].ok());
  EXPECT_EQ(*items[0], 7);
  EXPECT_FALSE(items[1].ok());
  EXPECT_EQ(items[1].status().code(), absl::StatusCode::kNotFound);
  EXPECT_EQ(items[1].status().message(), "missing");
}

TEST(StatusOrTrait, SharedFutureCopies) {
  auto [f, p] = yaclib::MakeSharedContract<int, Trait>();
  auto copy = f;
  std::move(p).Set(42);
  // StatusOr is copyable, so both SharedFuture copies observe the same value
  const auto& lvalue_result = f.Get();
  ASSERT_TRUE(lvalue_result.ok());
  EXPECT_EQ(*lvalue_result, 42);
  auto rvalue_result = std::move(copy).Get();
  static_assert(std::is_same_v<decltype(rvalue_result), absl::StatusOr<int>>);
  ASSERT_TRUE(rvalue_result.ok());
  EXPECT_EQ(*rvalue_result, 42);
}

#if YACLIB_CORO != 0

yaclib::Future<int, Trait> CoroReturn42() {
  co_return 42;
}

TEST(StatusOrTrait, CoroReturnValue) {
  auto future = CoroReturn42();
  EXPECT_TRUE(future.Ready());
  auto result = std::move(future).Touch();
  static_assert(std::is_same_v<decltype(result), absl::StatusOr<int>>);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(*result, 42);
}

yaclib::Future<int, Trait> CoroAwaitFailed() {
  auto inner = yaclib::MakeFuture<int, Trait>(absl::InvalidArgumentError("bad argument"));
  co_return co_await std::move(inner);
}

TEST(StatusOrTrait, CoroAwaitFailedFuture) {
  auto future = CoroAwaitFailed();
  auto result = std::move(future).Get();
  EXPECT_FALSE(result.ok());
  // co_await threw absl::BadStatusOrAccess, FromException catches it via the
  // std::exception path, so the outer status is kUnknown with the what() message
  EXPECT_EQ(result.status().code(), absl::StatusCode::kUnknown);
  EXPECT_NE(result.status().message().find("bad argument"), absl::string_view::npos);
}

yaclib::Task<int, Trait> CoroTask42() {
  co_return 42;
}

TEST(StatusOrTrait, CoroTask) {
  auto task = CoroTask42();
  auto result = std::move(task).Get();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(*result, 42);
}

TEST(StatusOrTrait, CoroTaskCancel) {
  auto task = CoroTask42();
  // Drop stores StopTag -> kCancelled, the dropped result is unobservable, just must not crash
  std::move(task).Cancel();
}

#endif

}  // namespace
}  // namespace test

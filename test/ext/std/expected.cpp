#include <version>

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L

#  include <yaclib_ext/std/expected.hpp>

#  include <yaclib/async/contract.hpp>
#  include <yaclib/async/future.hpp>
#  include <yaclib/async/make.hpp>
#  include <yaclib/async/promise.hpp>
#  include <yaclib/async/run.hpp>
#  include <yaclib/async/shared_contract.hpp>
#  include <yaclib/async/shared_future.hpp>
#  include <yaclib/async/shared_promise.hpp>
#  include <yaclib/async/when_all.hpp>
#  include <yaclib/config.hpp>
#  include <yaclib/runtime/fair_thread_pool.hpp>
#  include <yaclib/util/result.hpp>

#  if YACLIB_CORO != 0
#    include <yaclib/coro/await.hpp>
#    include <yaclib/coro/future.hpp>
#  endif

#  include <exception>
#  include <expected>
#  include <stdexcept>
#  include <system_error>
#  include <tuple>
#  include <type_traits>
#  include <utility>
#  include <vector>

#  include <gtest/gtest.h>

namespace test {
namespace {

/**
 * Plain std::error_code satisfying \ref yaclib_ext::ExpectedTrait requirements for E
 */
struct Error : std::error_code {
  using std::error_code::error_code;

  Error(std::error_code error) noexcept : std::error_code{error} {
  }

  Error(yaclib::StopTag /*tag*/) noexcept : std::error_code{std::make_error_code(std::errc::operation_canceled)} {
  }

  Error(std::exception_ptr e) noexcept : std::error_code{FromException(std::move(e))} {
  }

 private:
  static std::error_code FromException(std::exception_ptr e) noexcept {
    try {
      std::rethrow_exception(std::move(e));
    } catch (const yaclib::StopException&) {
      return std::make_error_code(std::errc::operation_canceled);
    } catch (const std::system_error& error) {
      return error.code();
    } catch (...) {
      return std::make_error_code(std::errc::io_error);
    }
  }
};

using Trait = yaclib_ext::ExpectedTrait<Error>;

template <typename V>
using R = std::expected<V, Error>;

static_assert(std::is_same_v<Trait::Result<int>, R<int>>);
static_assert(std::is_same_v<Trait::Error, Error>);

const Error kInvalidArgument{std::make_error_code(std::errc::invalid_argument)};
const Error kIoError{std::make_error_code(std::errc::io_error)};

TEST(ExpectedTrait, MakeResult) {
  auto value = Trait::MakeResult<int>(5);
  static_assert(std::is_same_v<decltype(value), R<int>>);
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(*value, 5);

  auto unit = Trait::MakeResult<int>(yaclib::Unit{});
  EXPECT_TRUE(unit.has_value());
  EXPECT_EQ(*unit, 0);

  auto stop = Trait::MakeResult<int>(yaclib::StopTag{});
  EXPECT_FALSE(stop.has_value());
  EXPECT_TRUE(Trait::IsStop(stop.error()));

  auto error = Trait::MakeResult<int>(std::make_exception_ptr(std::runtime_error{""}));
  EXPECT_FALSE(error.has_value());
  EXPECT_EQ(error.error(), kIoError);

  // std::system_error keeps its code, yaclib::StopException maps to cancellation
  auto system = Trait::MakeResult<int>(std::make_exception_ptr(std::system_error{kInvalidArgument}));
  EXPECT_EQ(system.error(), kInvalidArgument);
  auto stop_ptr = Trait::MakeResult<int>(std::make_exception_ptr(yaclib::StopException{}));
  EXPECT_TRUE(Trait::IsStop(stop_ptr.error()));

  auto pass = Trait::MakeResult<int>(R<int>{1});
  EXPECT_TRUE(pass.has_value());
  EXPECT_EQ(*pass, 1);

  auto in_place = Trait::MakeResult<int>(std::in_place, 2);
  EXPECT_TRUE(in_place.has_value());
  EXPECT_EQ(*in_place, 2);
}

TEST(ExpectedTrait, Accessors) {
  auto value = Trait::MakeResult<int>(5);
  EXPECT_TRUE(Trait::Ok(value));
  EXPECT_EQ(Trait::GetValue(std::as_const(value)), 5);
  EXPECT_EQ(Trait::Get(std::as_const(value)), 5);
  EXPECT_EQ(Trait::GetValue(std::move(value)), 5);

  auto stop = Trait::MakeResult<int>(yaclib::StopTag{});
  EXPECT_FALSE(Trait::Ok(stop));
  EXPECT_TRUE(Trait::IsStop(Trait::GetError(std::as_const(stop))));
  EXPECT_THROW(std::ignore = Trait::Get(std::move(stop)), std::bad_expected_access<Error>);
}

TEST(ExpectedTrait, IsStop) {
  EXPECT_TRUE(Trait::IsStop(Error{yaclib::StopTag{}}));
  EXPECT_FALSE(Trait::IsStop(kIoError));
}

TEST(ExpectedTrait, ContractValue) {
  auto [f, p] = yaclib::MakeContract<int, Trait>();
  EXPECT_FALSE(f.Ready());
  std::move(p).Set(5);
  EXPECT_TRUE(f.Ready());
  std::expected<int, Error> r = std::move(f).Get();
  // The whole point: the future yields a completely ordinary std::expected
  EXPECT_TRUE(r.has_value());
  EXPECT_EQ(*r, 5);
}

TEST(ExpectedTrait, ContractCancel) {
  auto [f, p] = yaclib::MakeContract<int, Trait>();
  {
    auto drop = std::move(p);  // ~Promise sets StopTag, represented in the trait's own error type
  }
  auto r = std::move(f).Get();
  EXPECT_FALSE(r.has_value());
  EXPECT_TRUE(Trait::IsStop(r.error()));
  EXPECT_EQ(r.error(), std::make_error_code(std::errc::operation_canceled));
}

TEST(ExpectedTrait, RunThen) {
  yaclib::FairThreadPool tp{2};
  auto r = yaclib::Run<Trait>(tp,
                              [] {
                                return 1;
                              })
             .Then([](int x) {
               return x + 1;
             })
             .Then([](int x) {
               return x * 3;
             })
             .Get();
  EXPECT_TRUE(r.has_value());
  EXPECT_EQ(*r, 6);
  tp.Stop();
  tp.Wait();
}

TEST(ExpectedTrait, RecoveryFromException) {
  // The throwing callback is stored via Error{std::exception_ptr},
  // the callback taking Error runs only on failure and recovers the value
  auto r = yaclib::MakeFuture<int, Trait>(1)
             .ThenInline([](int) -> int {
               throw std::runtime_error{"expected"};
             })
             .ThenInline([](Error e) {
               EXPECT_EQ(e, kIoError);  // std::runtime_error is not a std::system_error
               EXPECT_FALSE(Trait::IsStop(e));
               return 7;
             })
             .Get();
  EXPECT_TRUE(r.has_value());
  EXPECT_EQ(*r, 7);
}

TEST(ExpectedTrait, RecoveryFromCancel) {
  auto [f, p] = yaclib::MakeContract<int, Trait>();
  auto f2 = std::move(f).ThenInline([](Error e) {
    EXPECT_TRUE(Trait::IsStop(e));
    return -1;
  });
  {
    auto drop = std::move(p);
  }
  auto r = std::move(f2).Get();
  EXPECT_TRUE(r.has_value());
  EXPECT_EQ(*r, -1);
}

TEST(ExpectedTrait, SetError) {
  auto [f, p] = yaclib::MakeContract<int, Trait>();
  std::move(p).Set(kInvalidArgument);
  auto r = std::move(f).Get();
  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), kInvalidArgument);
}

TEST(ExpectedTrait, MakeFutureError) {
  auto r = yaclib::MakeFuture<int, Trait>(kInvalidArgument).Get();
  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), kInvalidArgument);
}

TEST(ExpectedTrait, WhenAllFirstFail) {
  auto f1 = yaclib::MakeFuture<int, Trait>(1);
  auto f2 = yaclib::MakeFuture<int, Trait>(kInvalidArgument);
  auto r = yaclib::WhenAll(std::move(f1), std::move(f2)).Get();
  static_assert(std::is_same_v<decltype(r), R<std::vector<int>>>);
  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), kInvalidArgument);
}

TEST(ExpectedTrait, WhenAllNone) {
  auto f1 = yaclib::MakeFuture<int, Trait>(1);
  auto f2 = yaclib::MakeFuture<int, Trait>(kInvalidArgument);
  auto r = yaclib::WhenAll<yaclib::FailPolicy::None>(std::move(f1), std::move(f2)).Get();
  static_assert(std::is_same_v<decltype(r), R<std::vector<R<int>>>>);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->size(), 2);
  EXPECT_TRUE((*r)[0].has_value());
  EXPECT_EQ(*(*r)[0], 1);
  EXPECT_FALSE((*r)[1].has_value());
  EXPECT_EQ((*r)[1].error(), kInvalidArgument);
}

#  if YACLIB_CORO != 0

yaclib::Future<int, Trait> CoroReturn() {
  co_return 42;
}

TEST(ExpectedTrait, CoroReturn) {
  auto r = CoroReturn().Get();
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(*r, 42);
}

yaclib::Future<int, Trait> CoroAwaitFailed() {
  auto inner = yaclib::MakeFuture<int, Trait>(kInvalidArgument);
  co_return co_await std::move(inner);
}

TEST(ExpectedTrait, CoroAwaitFailedFuture) {
  // co_await resumes via Trait::Get == std::expected::value(), which throws
  // std::bad_expected_access<Error>; unhandled_exception stores it through
  // Error{std::exception_ptr}, and since bad_expected_access is not a
  // std::system_error the original code is lost: the outer error is io_error
  auto r = CoroAwaitFailed().Get();
  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), kIoError);
  EXPECT_FALSE(Trait::IsStop(r.error()));
}

#  endif

TEST(ExpectedTrait, SharedFuture) {
  auto [f, p] = yaclib::MakeSharedContract<int, Trait>();
  auto copy = f;
  std::move(p).Set(5);
  const auto& r1 = f.Get();
  ASSERT_TRUE(r1.has_value());
  EXPECT_EQ(*r1, 5);
  // std::expected is copyable, every observer sees the same value
  auto r2 = std::move(copy).Get();
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ(*r2, 5);
}

}  // namespace
}  // namespace test

#endif

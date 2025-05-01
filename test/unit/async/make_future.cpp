#include <util/error_code.hpp>

#include <yaclib/async/make.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

struct Kek {
  explicit Kek(int x) : _x{x} {
  }
  explicit Kek(int x, std::string_view str) : _x{x}, _str{str} {
  }
  bool operator==(const Kek& other) const {
    return _x == other._x && _str == other._str;
  }

 private:
  int _x;
  std::string_view _str;
};

TEST(MakeReadyFuture, Void) {
  {
    yaclib::Future<> f = yaclib::MakeFuture();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<> f = yaclib::MakeFuture(yaclib::Unit{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<> f = yaclib::MakeFuture<void>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<void, LikeErrorCode> f = yaclib::MakeFuture<void, LikeErrorCode>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<> f = yaclib::MakeFuture<yaclib::Unit>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<void, LikeErrorCode> f = yaclib::MakeFuture<yaclib::Unit, LikeErrorCode>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
}

struct Default {
  Default() = default;
  Default(const Default&) = delete;
  Default(Default&&) = delete;
};

TEST(MakeReadyFuture, Default) {
  {
    yaclib::Future<Default> f = yaclib::MakeFuture<Default>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
  }
  {
    yaclib::Future<Default, LikeErrorCode> f = yaclib::MakeFuture<Default, LikeErrorCode>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
  }
}

TEST(MakeReadyFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    const int x = 1;
    yaclib::Future<int> f = yaclib::MakeFuture(x);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    int x = 1;
    yaclib::Future<int> f = yaclib::MakeFuture(x);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    const int x = 1;
    yaclib::Future<int> f = yaclib::MakeFuture(std::move(x));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    int x = 1;
    yaclib::Future<int> f = yaclib::MakeFuture(std::move(x));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    yaclib::Future<int, LikeErrorCode> f = yaclib::MakeFuture<int, LikeErrorCode>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
}

TEST(MakeReadyFuture, Args1) {
  Kek kek{1};
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f = yaclib::MakeFuture<Kek, LikeErrorCode>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
}

TEST(MakeReadyFuture, Args2) {
  Kek kek{2, "rara"};
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(2, "rara");
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f = yaclib::MakeFuture<Kek, LikeErrorCode>(2, "rara");
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
}

TEST(MakeExceptionFuture, Void) {
  {
    yaclib::Future<> f = yaclib::MakeFuture<void>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Future<void, LikeErrorCode> f =
      yaclib::MakeFuture<void, LikeErrorCode>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
}

TEST(MakeExceptionFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Future<int, LikeErrorCode> f =
      yaclib::MakeFuture<int, LikeErrorCode>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
}

TEST(MakeExceptionFuture, NonTrivial) {
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f =
      yaclib::MakeFuture<Kek, LikeErrorCode>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
}

TEST(MakeErrorFuture, Void) {
  {
    yaclib::Future<> f = yaclib::MakeFuture<void>(yaclib::StopError{yaclib::StopTag{}});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Future<void, LikeErrorCode> f = yaclib::MakeFuture<void, LikeErrorCode>(LikeErrorCode{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeErrorFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(yaclib::StopError{yaclib::StopTag{}});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Future<int, LikeErrorCode> f = yaclib::MakeFuture<int, LikeErrorCode>(LikeErrorCode{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeErrorFuture, NonTrivial) {
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(yaclib::StopError{yaclib::StopTag{}});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f = yaclib::MakeFuture<Kek, LikeErrorCode>(LikeErrorCode{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeStoppedFuture, Void) {
  {
    yaclib::Future<> f = yaclib::MakeFuture<void>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Future<void, LikeErrorCode> f = yaclib::MakeFuture<void, LikeErrorCode>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeStoppedFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Future<int, LikeErrorCode> f = yaclib::MakeFuture<int, LikeErrorCode>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeStoppedFuture, NonTrivial) {
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f = yaclib::MakeFuture<Kek, LikeErrorCode>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

}  // namespace
}  // namespace test

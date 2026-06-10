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
    yaclib::Future<void, ErrorCodeTrait> f = yaclib::MakeFuture<void, ErrorCodeTrait>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<> f = yaclib::MakeFuture<yaclib::Unit>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Future<void, ErrorCodeTrait> f = yaclib::MakeFuture<yaclib::Unit, ErrorCodeTrait>();
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
    yaclib::Future<Default, ErrorCodeTrait> f = yaclib::MakeFuture<Default, ErrorCodeTrait>();
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
    yaclib::Future<int, ErrorCodeTrait> f = yaclib::MakeFuture<int, ErrorCodeTrait>(1);
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
    yaclib::Future<Kek, ErrorCodeTrait> f = yaclib::MakeFuture<Kek, ErrorCodeTrait>(1);
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
    yaclib::Future<Kek, ErrorCodeTrait> f = yaclib::MakeFuture<Kek, ErrorCodeTrait>(2, "rara");
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
    // ErrorCodeTrait converts std::exception_ptr to LikeErrorCode, so Ok() throws std::system_error
    yaclib::Future<void, ErrorCodeTrait> f =
      yaclib::MakeFuture<void, ErrorCodeTrait>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::io_error));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeExceptionFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Future<int, ErrorCodeTrait> f =
      yaclib::MakeFuture<int, ErrorCodeTrait>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::io_error));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeExceptionFuture, NonTrivial) {
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Future<Kek, ErrorCodeTrait> f =
      yaclib::MakeFuture<Kek, ErrorCodeTrait>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::io_error));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

// For the default trait Error == Stopped (single error channel), covered by MakeStoppedFuture;
// a non-stop exception_ptr error is covered by MakeExceptionFuture
TEST(MakeErrorFuture, Void) {
  {
    yaclib::Future<void, ErrorCodeTrait> f =
      yaclib::MakeFuture<void, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::invalid_argument));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeErrorFuture, Int) {
  {
    yaclib::Future<int, ErrorCodeTrait> f =
      yaclib::MakeFuture<int, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::invalid_argument));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeErrorFuture, NonTrivial) {
  {
    yaclib::Future<Kek, ErrorCodeTrait> f =
      yaclib::MakeFuture<Kek, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::invalid_argument));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeStoppedFuture, Void) {
  {
    yaclib::Future<> f = yaclib::MakeFuture<void>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::StopException);
  }
  {
    yaclib::Future<void, ErrorCodeTrait> f = yaclib::MakeFuture<void, ErrorCodeTrait>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), LikeErrorCode{yaclib::StopTag{}});
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeStoppedFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::StopException);
  }
  {
    yaclib::Future<int, ErrorCodeTrait> f = yaclib::MakeFuture<int, ErrorCodeTrait>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), LikeErrorCode{yaclib::StopTag{}});
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeStoppedFuture, NonTrivial) {
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::StopException);
  }
  {
    yaclib::Future<Kek, ErrorCodeTrait> f = yaclib::MakeFuture<Kek, ErrorCodeTrait>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), LikeErrorCode{yaclib::StopTag{}});
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

}  // namespace
}  // namespace test

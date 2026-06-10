#include <util/error_code.hpp>

#include <yaclib/lazy/make.hpp>

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

TEST(MakeReadyTask, Void) {
  {
    yaclib::Task<> f = yaclib::MakeTask();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<> f = yaclib::MakeTask<>(yaclib::Unit{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<> f = yaclib::MakeTask<void>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<void, ErrorCodeTrait> f = yaclib::MakeTask<void, ErrorCodeTrait>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<> f = yaclib::MakeTask<void>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<void, ErrorCodeTrait> f = yaclib::MakeTask<void, ErrorCodeTrait>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
}

struct Default {
  Default() = default;
  Default(const Default&) = delete;
  Default(Default&&) = delete;
};

TEST(MakeReadyTask, Default) {
  {
    yaclib::Task<Default> f = yaclib::MakeTask<Default>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
  }
  {
    yaclib::Task<Default, ErrorCodeTrait> f = yaclib::MakeTask<Default, ErrorCodeTrait>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
  }
}

TEST(MakeReadyTask, Int) {
  {
    yaclib::Task<int> f = yaclib::MakeTask(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    const int x = 1;
    yaclib::Task<int> f = yaclib::MakeTask(x);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    int x = 1;
    yaclib::Task<int> f = yaclib::MakeTask(x);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    const int x = 1;
    yaclib::Task<int> f = yaclib::MakeTask(std::move(x));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    int x = 1;
    yaclib::Task<int> f = yaclib::MakeTask(std::move(x));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    yaclib::Task<int> f = yaclib::MakeTask<int>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    yaclib::Task<int, ErrorCodeTrait> f = yaclib::MakeTask<int, ErrorCodeTrait>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
}

TEST(MakeReadyTask, Args1) {
  Kek kek{1};
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
  {
    yaclib::Task<Kek, ErrorCodeTrait> f = yaclib::MakeTask<Kek, ErrorCodeTrait>(1);
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
}

TEST(MakeReadyTask, Args2) {
  Kek kek{2, "rara"};
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(2, "rara");
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
  {
    yaclib::Task<Kek, ErrorCodeTrait> f = yaclib::MakeTask<Kek, ErrorCodeTrait>(2, "rara");
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
}

TEST(MakeExceptionTask, Void) {
  {
    yaclib::Task<> f = yaclib::MakeTask<void>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    // ErrorCodeTrait converts exception_ptr into LikeErrorCode, so Ok() throws std::system_error
    yaclib::Task<void, ErrorCodeTrait> f =
      yaclib::MakeTask<void, ErrorCodeTrait>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::io_error));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeExceptionTask, Int) {
  {
    yaclib::Task<int> f = yaclib::MakeTask<int>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Task<int, ErrorCodeTrait> f =
      yaclib::MakeTask<int, ErrorCodeTrait>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::io_error));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeExceptionTask, NonTrivial) {
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Task<Kek, ErrorCodeTrait> f =
      yaclib::MakeTask<Kek, ErrorCodeTrait>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::io_error));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

// For the default trait Error == Stopped (single error channel), covered by MakeStoppedTask;
// a non-stop exception_ptr error is covered by MakeExceptionTask
TEST(MakeErrorTask, Void) {
  {
    yaclib::Task<void, ErrorCodeTrait> f =
      yaclib::MakeTask<void, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::invalid_argument));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeErrorTask, Int) {
  {
    yaclib::Task<int, ErrorCodeTrait> f =
      yaclib::MakeTask<int, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::invalid_argument));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeErrorTask, NonTrivial) {
  {
    yaclib::Task<Kek, ErrorCodeTrait> f =
      yaclib::MakeTask<Kek, ErrorCodeTrait>(LikeErrorCode{std::make_error_code(std::errc::invalid_argument)});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), std::make_error_code(std::errc::invalid_argument));
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeStoppedTask, Void) {
  {
    yaclib::Task<> f = yaclib::MakeTask<void>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::StopException);
  }
  {
    yaclib::Task<void, ErrorCodeTrait> f = yaclib::MakeTask<void, ErrorCodeTrait>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), LikeErrorCode{yaclib::StopTag{}});
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeStoppedTask, Int) {
  {
    yaclib::Task<int> f = yaclib::MakeTask<int>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::StopException);
  }
  {
    yaclib::Task<int, ErrorCodeTrait> f = yaclib::MakeTask<int, ErrorCodeTrait>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), LikeErrorCode{yaclib::StopTag{}});
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeStoppedTask, NonTrivial) {
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::StopException);
  }
  {
    yaclib::Task<Kek, ErrorCodeTrait> f = yaclib::MakeTask<Kek, ErrorCodeTrait>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    auto result = std::move(f).Get();
    EXPECT_EQ(result.Error(), LikeErrorCode{yaclib::StopTag{}});
    EXPECT_THROW(std::ignore = std::move(result).Ok(), std::system_error);
  }
}

TEST(MakeTask, ThrowingConstruction) {
  struct ThrowOnCopy {
    ThrowOnCopy() = default;
    ThrowOnCopy(const ThrowOnCopy&) {
      throw std::runtime_error{"copy"};
    }
    ThrowOnCopy(ThrowOnCopy&&) = default;
    std::string owner{"owner"};
  };
  ThrowOnCopy value;
  // The throwing copy must become the stored error, not unwind through the core
  auto task = yaclib::MakeTask<ThrowOnCopy>(value);
  auto result = std::move(task).Get();
  EXPECT_FALSE(result);
  EXPECT_THROW(std::rethrow_exception(std::as_const(result).Error()), std::runtime_error);
}

}  // namespace
}  // namespace test

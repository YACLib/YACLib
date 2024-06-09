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
    yaclib::Task<void, LikeErrorCode> f = yaclib::MakeTask<void, LikeErrorCode>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<> f = yaclib::MakeTask<void>();
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }
  {
    yaclib::Task<void, LikeErrorCode> f = yaclib::MakeTask<void, LikeErrorCode>();
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
    yaclib::Task<Default, LikeErrorCode> f = yaclib::MakeTask<Default, LikeErrorCode>();
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
    yaclib::Task<int, LikeErrorCode> f = yaclib::MakeTask<int, LikeErrorCode>(1);
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
    yaclib::Task<Kek, LikeErrorCode> f = yaclib::MakeTask<Kek, LikeErrorCode>(1);
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
    yaclib::Task<Kek, LikeErrorCode> f = yaclib::MakeTask<Kek, LikeErrorCode>(2, "rara");
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
    yaclib::Task<void, LikeErrorCode> f =
      yaclib::MakeTask<void, LikeErrorCode>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
}

TEST(MakeExceptionTask, Int) {
  {
    yaclib::Task<int> f = yaclib::MakeTask<int>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Task<int, LikeErrorCode> f =
      yaclib::MakeTask<int, LikeErrorCode>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
}

TEST(MakeExceptionTask, NonTrivial) {
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
  {
    yaclib::Task<Kek, LikeErrorCode> f =
      yaclib::MakeTask<Kek, LikeErrorCode>(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), std::runtime_error);
  }
}

TEST(MakeErrorTask, Void) {
  {
    yaclib::Task<> f = yaclib::MakeTask<void>(yaclib::StopError{yaclib::StopTag{}});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Task<void, LikeErrorCode> f = yaclib::MakeTask<void, LikeErrorCode>(LikeErrorCode{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeErrorTask, Int) {
  {
    yaclib::Task<int> f = yaclib::MakeTask<int>(yaclib::StopError{yaclib::StopTag{}});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Task<int, LikeErrorCode> f = yaclib::MakeTask<int, LikeErrorCode>(LikeErrorCode{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeErrorTask, NonTrivial) {
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(yaclib::StopError{yaclib::StopTag{}});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Task<Kek, LikeErrorCode> f = yaclib::MakeTask<Kek, LikeErrorCode>(LikeErrorCode{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeStoppedTask, Void) {
  {
    yaclib::Task<> f = yaclib::MakeTask<void>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Task<void, LikeErrorCode> f = yaclib::MakeTask<void, LikeErrorCode>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeStoppedTask, Int) {
  {
    yaclib::Task<int> f = yaclib::MakeTask<int>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Task<int, LikeErrorCode> f = yaclib::MakeTask<int, LikeErrorCode>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

TEST(MakeStoppedTask, NonTrivial) {
  {
    yaclib::Task<Kek> f = yaclib::MakeTask<Kek>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<yaclib::StopError>);
  }
  {
    yaclib::Task<Kek, LikeErrorCode> f = yaclib::MakeTask<Kek, LikeErrorCode>(yaclib::StopTag{});
    EXPECT_EQ(f.GetCore()->_executor, &yaclib::MakeInline());
    EXPECT_THROW(std::ignore = std::move(f).Get().Ok(), yaclib::ResultError<LikeErrorCode>);
  }
}

}  // namespace
}  // namespace test

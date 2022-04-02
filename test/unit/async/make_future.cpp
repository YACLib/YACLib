#include <util/error_code.hpp>

#include <yaclib/async/future.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(MakeFuture, Void) {
  {
    yaclib::Future<void> f = yaclib::MakeFuture();
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }
  {
    yaclib::Future<void> f = yaclib::MakeFuture<void>();
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }
  {
    yaclib::Future<void, LikeErrorCode> f = yaclib::MakeFuture<void, LikeErrorCode>();
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }
}

TEST(MakeFuture, Int) {
  {
    yaclib::Future<int> f = yaclib::MakeFuture(1);
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    yaclib::Future<int> f = yaclib::MakeFuture<int>(1);
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
  {
    yaclib::Future<int, LikeErrorCode> f = yaclib::MakeFuture<int, LikeErrorCode>(1);
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), 1);
  }
}

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

TEST(MakeFuture, Args1) {
  Kek kek{1};
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(1);
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f = yaclib::MakeFuture<Kek, LikeErrorCode>(1);
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
}

TEST(MakeFuture, Args2) {
  Kek kek{2, "rara"};
  {
    yaclib::Future<Kek> f = yaclib::MakeFuture<Kek>(2, "rara");
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
  {
    yaclib::Future<Kek, LikeErrorCode> f = yaclib::MakeFuture<Kek, LikeErrorCode>(2, "rara");
    EXPECT_EQ(f.GetCore()->GetExecutor(), nullptr);
    EXPECT_EQ(std::move(f).Get().Ok(), kek);
  }
}

}  // namespace
}  // namespace test

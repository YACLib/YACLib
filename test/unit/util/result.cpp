#include <util/error_code.hpp>

#include <yaclib/util/result.hpp>

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {

struct NotDefaultConstructible {
  NotDefaultConstructible() = delete;
  NotDefaultConstructible(int) {
  }
};

TEST(Result, Simple) {
  yaclib::Result<int> result;
  EXPECT_FALSE(result);
  result = 5;
  EXPECT_TRUE(result);
  EXPECT_EQ(std::move(result).Ok(), 5);
  {
    yaclib::Result<int> result2 = std::move(result);
    yaclib::Result<int> result3 = result2;
    result = std::move(result2);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result3);
    EXPECT_EQ(std::move(result3).Ok(), 5);
  }
}

TEST(Result, DefaultIsStop) {
  yaclib::Result<int> result;
  EXPECT_FALSE(result);
  EXPECT_TRUE(yaclib::IsStop(std::as_const(result).Error()));
  try {
    std::ignore = std::move(result).Ok();
    FAIL();
  } catch (const yaclib::StopException& e) {
    EXPECT_STREQ(e.what(), "yaclib::StopException");
  }
}

TEST(Result, StopTag) {
  yaclib::Result<int> result{yaclib::StopTag{}};
  EXPECT_FALSE(result);
  EXPECT_TRUE(yaclib::IsStop(std::as_const(result).Error()));
  EXPECT_THROW(std::ignore = std::move(result).Ok(), yaclib::StopException);
}

TEST(Result, Value) {
  yaclib::Result<std::string> result{"kek"};
  EXPECT_TRUE(result);
  EXPECT_EQ(std::as_const(result).Value(), "kek");
  EXPECT_EQ(std::as_const(result).Ok(), "kek");
  EXPECT_EQ(std::move(result).Value(), "kek");
}

TEST(Result, InPlace) {
  yaclib::Result<std::string> result{std::in_place, 3, 'a'};
  EXPECT_TRUE(result);
  EXPECT_EQ(std::move(result).Ok(), "aaa");

  yaclib::Result<NotDefaultConstructible> result2{std::in_place, 1};
  EXPECT_TRUE(result2);

  yaclib::Result<NotDefaultConstructible> result3;
  result3 = NotDefaultConstructible{5};
  EXPECT_TRUE(result3);
}

TEST(Result, Exception) {
  yaclib::Result<int> result{std::make_exception_ptr(std::runtime_error{"kek"})};
  EXPECT_FALSE(result);
  EXPECT_FALSE(yaclib::IsStop(std::as_const(result).Error()));
  EXPECT_THROW(std::ignore = std::move(result).Ok(), std::runtime_error);
}

TEST(Result, AssignDispatch) {
  yaclib::Result<int> result{5};
  result = std::make_exception_ptr(std::runtime_error{""});
  EXPECT_FALSE(result);
  result = 7;
  EXPECT_TRUE(result);
  EXPECT_EQ(std::as_const(result).Value(), 7);
  result = yaclib::StopTag{};
  EXPECT_FALSE(result);
  EXPECT_TRUE(yaclib::IsStop(std::as_const(result).Error()));
}

TEST(Result, CopyMoveTransitions) {
  yaclib::Result<std::string> value{"value"};
  yaclib::Result<std::string> error{yaclib::StopTag{}};

  yaclib::Result<std::string> result;
  result = value;  // error -> value
  EXPECT_TRUE(result);
  EXPECT_EQ(std::as_const(result).Value(), "value");
  result = error;  // value -> error
  EXPECT_FALSE(result);
  result = std::move(value);  // error -> value
  EXPECT_TRUE(result);
  EXPECT_EQ(std::as_const(result).Value(), "value");
  result = yaclib::Result<std::string>{"other"};  // value -> value
  EXPECT_EQ(std::as_const(result).Value(), "other");

  const yaclib::Result<std::string> another{"another"};
  result = another;  // value -> value copy
  EXPECT_EQ(std::as_const(result).Value(), "another");

  auto& self = result;  // self assignment in both states, reference to silence -Wself-assign
  result = self;
  EXPECT_EQ(std::as_const(result).Value(), "another");
  result = std::move(self);
  EXPECT_TRUE(result);
  auto& error_self = error;
  error = error_self;
  EXPECT_FALSE(error);
  EXPECT_TRUE(yaclib::IsStop(std::as_const(error).Error()));

  // moved-from error result still holds the error: _error is copied, not moved
  yaclib::Result<std::string> moved{std::move(error)};
  EXPECT_FALSE(moved);
  EXPECT_FALSE(error);
  EXPECT_TRUE(yaclib::IsStop(std::as_const(error).Error()));
}

TEST(Result, MoveOnly) {
  yaclib::Result<std::unique_ptr<int>> result{std::make_unique<int>(5)};
  EXPECT_TRUE(result);
  auto moved = std::move(result);
  EXPECT_EQ(*std::as_const(moved).Value(), 5);
  auto ptr = std::move(moved).Ok();
  EXPECT_EQ(*ptr, 5);
}

TEST(Result, Void) {
  yaclib::Result<> result;
  EXPECT_FALSE(result);
  result = yaclib::Unit{};
  EXPECT_TRUE(result);
  std::ignore = std::move(result).Ok();
}

#ifndef YACLIB_LOG_DEBUG
TEST(Result, NullExceptionPtrIsStop) {
  // Precondition violation: with asserts enabled YACLIB_ASSERT reports it,
  // without asserts the null error degrades to the stop error instead of breaking the value lifetime
  yaclib::Result<int> result{std::exception_ptr{}};
  EXPECT_FALSE(result);
  EXPECT_TRUE(yaclib::IsStop(std::as_const(result).Error()));
}
#endif

TEST(Result, StopPtr) {
  EXPECT_EQ(&yaclib::StopPtr(), &yaclib::StopPtr());
  EXPECT_TRUE(yaclib::IsStop(yaclib::StopPtr()));
  // IsStop also recognizes a separately created StopException
  EXPECT_TRUE(yaclib::IsStop(std::make_exception_ptr(yaclib::StopException{})));
  EXPECT_FALSE(yaclib::IsStop(std::make_exception_ptr(std::runtime_error{""})));
  EXPECT_FALSE(yaclib::IsStop(std::exception_ptr{}));
}

template <typename T>
void TestTrait() {
  using R = typename T::template Result<int>;

  auto value = T::template MakeResult<int>(5);
  EXPECT_TRUE(T::Ok(value));
  EXPECT_EQ(T::GetValue(std::as_const(value)), 5);
  EXPECT_EQ(T::GetValue(std::move(value)), 5);

  auto stop = T::template MakeResult<int>(yaclib::StopTag{});
  EXPECT_FALSE(T::Ok(stop));
  std::ignore = T::GetError(std::as_const(stop));
  std::ignore = T::GetError(std::move(stop));

  auto unit = T::template MakeResult<int>(yaclib::Unit{});
  EXPECT_TRUE(T::Ok(unit));
  EXPECT_EQ(T::GetValue(std::move(unit)), 0);

  auto error = T::template MakeResult<int>(std::make_exception_ptr(std::runtime_error{""}));
  EXPECT_FALSE(T::Ok(error));

  auto pass = T::template MakeResult<int>(R{1});
  EXPECT_TRUE(T::Ok(pass));
  EXPECT_EQ(T::GetValue(std::move(pass)), 1);

  auto in_place = T::template MakeResult<int>(std::in_place, 2);
  EXPECT_TRUE(T::Ok(in_place));
  EXPECT_EQ(T::GetValue(std::move(in_place)), 2);
}

TEST(ResultTrait, Default) {
  TestTrait<yaclib::DefaultTrait>();
}

TEST(ResultTrait, ErrorCode) {
  TestTrait<ErrorCodeTrait>();
}

}  // namespace
}  // namespace test

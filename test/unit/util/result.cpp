#include <util/error_code.hpp>

#include <yaclib/util/result.hpp>

#include <exception>
#include <stdexcept>
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

TEST(Simple, Simple) {
  yaclib::Result<int> result;
  EXPECT_EQ(result.State(), yaclib::ResultState::Empty);
  result = 5;
  EXPECT_EQ(result.State(), yaclib::ResultState::Value);
  EXPECT_EQ(std::move(result).Ok(), 5);
  {
    yaclib::Result<int> result2 = std::move(result);
    yaclib::Result<int> result3 = result2;
    result = std::move(result2);
    result = result3;
  }
  {
    yaclib::StopTag tag;
    yaclib::StopError error{tag};
    yaclib::Result<int> result2 = tag;
    yaclib::Result<int> result3 = error;
    yaclib::Result<int> result4 = std::move(tag);
    yaclib::Result<int> result5 = std::move(error);
    result2 = result3;
    result4 = std::move(result2);
  }
}

TEST(Simple, NotDefaultConstructible) {
  yaclib::Result<NotDefaultConstructible> result;
  result = NotDefaultConstructible{5};
}

void TestState(yaclib::ResultState state) {
  yaclib::Result<int> result;
  EXPECT_EQ(result.State(), yaclib::ResultState::Empty);
  switch (state) {
    case yaclib::ResultState::Value: {
      result = 1;
    } break;
    case yaclib::ResultState::Error: {
      result = yaclib::StopTag{};
    } break;
    case yaclib::ResultState::Exception: {
      result = std::make_exception_ptr(std::runtime_error{""});
    } break;
    case yaclib::ResultState::Empty: {
    } break;
  }
  EXPECT_EQ(result.State(), state);
}

TEST(State, Empty) {
  TestState(yaclib::ResultState::Empty);
}

TEST(State, Error) {
  TestState(yaclib::ResultState::Error);
}

TEST(State, Exception) {
  TestState(yaclib::ResultState::Exception);
}

TEST(State, Value) {
  TestState(yaclib::ResultState::Value);
}

template <typename Result>
void TestOk(Result&& result, yaclib::ResultState state) {
  EXPECT_EQ(result.State(), yaclib::ResultState::Empty);
  switch (state) {
    case yaclib::ResultState::Value: {
      result = "1";
      EXPECT_EQ(std::as_const(result).Ok(), "1");
    } break;
    case yaclib::ResultState::Error: {
      result = yaclib::StopTag{};
      try {
        std::ignore = std::as_const(result).Ok();
      } catch (const yaclib::ResultError<yaclib::StopError>& e) {
        EXPECT_STREQ(e.what(), "yaclib::StopError");
      } catch (const yaclib::ResultError<LikeErrorCode>& e) {
        EXPECT_STREQ(e.what(), "generic");
      }
    } break;
    case yaclib::ResultState::Exception: {
      result = std::make_exception_ptr(std::runtime_error{""});
      EXPECT_THROW(std::ignore = std::as_const(result).Ok(), std::runtime_error);
    } break;
    case yaclib::ResultState::Empty: {
      try {
        std::ignore = std::as_const(result).Ok();
      } catch (const yaclib::ResultEmpty& e) {
        EXPECT_STREQ(e.what(), "yaclib::ResultEmpty");
      }
    } break;
  }
  EXPECT_EQ(result.State(), state);
  result = {};
}

TEST(Ok, Value) {
  yaclib::Result<std::string> r1;
  TestOk(r1, yaclib::ResultState::Value);
  TestOk(std::move(r1), yaclib::ResultState::Value);

  yaclib::Result<std::string, LikeErrorCode> r2;
  TestOk(r2, yaclib::ResultState::Value);
  TestOk(std::move(r2), yaclib::ResultState::Value);
}

TEST(Ok, Error) {
  yaclib::Result<std::string> r1;
  TestOk(r1, yaclib::ResultState::Error);
  TestOk(std::move(r1), yaclib::ResultState::Error);

  yaclib::Result<std::string, LikeErrorCode> r2;
  TestOk(r2, yaclib::ResultState::Error);
  TestOk(std::move(r2), yaclib::ResultState::Error);
}

TEST(Ok, Exception) {
  yaclib::Result<std::string> r1;
  TestOk(r1, yaclib::ResultState::Exception);
  TestOk(std::move(r1), yaclib::ResultState::Exception);

  yaclib::Result<std::string, LikeErrorCode> r2;
  TestOk(r2, yaclib::ResultState::Exception);
  TestOk(std::move(r2), yaclib::ResultState::Exception);
}

TEST(Ok, Empty) {
  yaclib::Result<std::string> r1;
  TestOk(r1, yaclib::ResultState::Empty);
  TestOk(std::move(r1), yaclib::ResultState::Empty);

  yaclib::Result<std::string, LikeErrorCode> r2;
  TestOk(r2, yaclib::ResultState::Empty);
  TestOk(std::move(r2), yaclib::ResultState::Empty);
}

}  // namespace
}  // namespace test

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
template <typename E = yaclib::StopError>
void TestOk(yaclib::ResultState state) {
  yaclib::Result<int, E> result;
  EXPECT_EQ(result.State(), yaclib::ResultState::Empty);
  switch (state) {
    case yaclib::ResultState::Value: {
      result = 1;
      EXPECT_EQ(std::move(result).Ok(), 1);
    } break;
    case yaclib::ResultState::Error: {
      result = yaclib::StopTag{};
      EXPECT_THROW(std::move(result).Ok(), yaclib::ResultError<E>);
    } break;
    case yaclib::ResultState::Exception: {
      result = std::make_exception_ptr(std::runtime_error{""});
      EXPECT_THROW(std::move(result).Ok(), std::runtime_error);
    } break;
    case yaclib::ResultState::Empty: {
      EXPECT_THROW(std::move(result).Ok(), yaclib::ResultEmpty);
    } break;
  }
  EXPECT_EQ(result.State(), state);
}

TEST(Ok, Value) {
  TestOk(yaclib::ResultState::Value);
  TestOk<LikeErrorCode>(yaclib::ResultState::Value);
}

TEST(Ok, Error) {
  TestOk(yaclib::ResultState::Error);
  TestOk<LikeErrorCode>(yaclib::ResultState::Error);
}

TEST(Ok, Exception) {
  TestOk(yaclib::ResultState::Exception);
  TestOk<LikeErrorCode>(yaclib::ResultState::Exception);
}

TEST(Ok, Empty) {
  TestOk(yaclib::ResultState::Empty);
  TestOk<LikeErrorCode>(yaclib::ResultState::Empty);
}

}  // namespace
}  // namespace test

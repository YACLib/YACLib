#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace {

using namespace yaclib::util;

struct NotDefaultConstructible {
  NotDefaultConstructible() = delete;
  NotDefaultConstructible(int) {
  }
};

TEST(Simple, Simple) {
  Result<int> result;
  EXPECT_EQ(result.State(), ResultState::Empty);
  result.Set(5);
  EXPECT_EQ(result.State(), ResultState::Value);
  EXPECT_EQ(std::move(result).Ok(), 5);
}

TEST(Simple, NotDefaultConstructible) {
  Result<NotDefaultConstructible> result;
  result.Set(NotDefaultConstructible{5});
}

void TestState(ResultState state) {
  Result<int> result;
  EXPECT_EQ(result.State(), ResultState::Empty);
  switch (state) {
    case ResultState::Value: {
      std::move(result).Set(1);
    } break;
    case ResultState::Error: {
      std::move(result).Set(std::error_code{});
    } break;
    case ResultState::Exception: {
      std::move(result).Set(std::make_exception_ptr(std::runtime_error{""}));
    } break;
    case ResultState::Empty: {
    } break;
  }
  EXPECT_EQ(result.State(), state);
}

TEST(State, Empty) {
  TestState(ResultState::Empty);
}

TEST(State, Error) {
  TestState(ResultState::Error);
}

TEST(State, Exception) {
  TestState(ResultState::Exception);
}

TEST(State, Value) {
  TestState(ResultState::Value);
}

void TestOk(ResultState state) {
  Result<int> result;
  EXPECT_EQ(result.State(), ResultState::Empty);
  switch (state) {
    case ResultState::Value: {
      std::move(result).Set(1);
      EXPECT_NO_THROW(std::move(result).Ok());
    } break;
    case ResultState::Error: {
      std::move(result).Set(std::error_code{});
      EXPECT_THROW(std::move(result).Ok(), ResultError);
    } break;
    case ResultState::Exception: {
      std::move(result).Set(std::make_exception_ptr(std::runtime_error{""}));
      EXPECT_THROW(std::move(result).Ok(), std::runtime_error);
    } break;
    case ResultState::Empty: {
      EXPECT_THROW(std::move(result).Ok(), ResultEmpty);
    } break;
  }
  EXPECT_EQ(result.State(), state);
}

TEST(Ok, Value) {
  TestOk(ResultState::Value);
}

TEST(Ok, Error) {
  TestOk(ResultState::Error);
}

TEST(Ok, Exception) {
  TestOk(ResultState::Exception);
}

TEST(Ok, Empty) {
  TestOk(ResultState::Empty);
}

}  // namespace

#include <yaclib/log_config.hpp>

#include <gtest/gtest.h>

#define GTEST_COUT std::cerr << "[          ] [ INFO ] "

class LoggingTest : public ::testing::Test {
 protected:
  void TearDown() override {
    SetErrorCallback([](std::string_view file, std::size_t line, std::string_view /*function*/,
                        std::string_view /*condition*/, std::string_view message) {
      GTEST_MESSAGE_AT_(file.data(), line, message.data(), ::testing::TestPartResult::kFatalFailure);
    });
    SetInfoCallback([](std::string_view file, std::size_t line, std::string_view function,
                       std::string_view /*condition*/, std::string_view message) {
      GTEST_COUT << message << " in" << file << ":" << line << ". Function name: " << function;
    });
  }
};

TEST_F(LoggingTest, Error) {
  std::stringstream result;
  SetErrorCallback([&result](std::string_view file, std::size_t line, std::string_view function,
                             std::string_view condition, std::string_view message) {
    result << file << ":" << line << " " << function << " " << condition << " :kek: " << message;
  });
  YACLIB_ERROR(1 != 2, "1 != 2");
  YACLIB_ERROR(1 == 2, "1 == 2");
  std::stringstream expected;
  expected << __FILE__ << ":" << 27 << " " << YACLIB_FUNCTION_NAME << " "
           << "1 != 2"
           << " :kek: "
           << "1 != 2";
  ASSERT_EQ(result.str(), expected.str());
}

TEST_F(LoggingTest, Info) {
  std::stringstream result;
  SetInfoCallback([&result](std::string_view file, std::size_t line, std::string_view function,
                            std::string_view condition, std::string_view message) {
    result << file << ":" << line << " " << function << " " << condition << " :kek: " << message;
  });
  YACLIB_INFO(1 != 2, "1 != 2");
  YACLIB_INFO(1 == 2, "1 == 2");
  std::stringstream expected;
  expected << __FILE__ << ":" << 43 << " " << YACLIB_FUNCTION_NAME << " "
           << "1 != 2"
           << " :kek: "
           << "1 != 2";
  ASSERT_EQ(result.str(), expected.str());
}

TEST_F(LoggingTest, WithoutCallback) {
  SetErrorCallback(nullptr);
  SetInfoCallback(nullptr);
  ASSERT_NO_FATAL_FAILURE({
    YACLIB_INFO(1 != 2, "1 != 2");
    YACLIB_ERROR(1 != 2, "1 != 2");
  });
}

TEST_F(LoggingTest, Resetcallback) {
  std::stringstream result_error;
  SetErrorCallback([&result_error](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                                   std::string_view /*condition*/, std::string_view /*message*/) {
    result_error << "lol";
  });
  YACLIB_ERROR(true, "lol");
  SetErrorCallback([&result_error](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                                   std::string_view /*condition*/, std::string_view /*message*/) {
    result_error << "kek";
  });
  YACLIB_ERROR(true, "kek");

  ASSERT_EQ(result_error.str(), "lolkek");

  std::stringstream result_info;
  SetInfoCallback([&result_info](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                                 std::string_view /*condition*/, std::string_view /*message*/) {
    result_info << "lol";
  });
  YACLIB_INFO(true, "lol");
  SetInfoCallback([&result_info](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                                 std::string_view /*condition*/, std::string_view /*message*/) {
    result_info << "kek";
  });
  YACLIB_INFO(true, "kek");
  ASSERT_EQ(result_info.str(), "lolkek");
}

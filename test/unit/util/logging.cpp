#include <yaclib/log_config.hpp>

#include <fstream>

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
    std::remove("resultError");
    std::remove("resultInfo");
    std::remove("resultResetCallback");
  }
};

TEST_F(LoggingTest, Error) {
  SetErrorCallback([](std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
                      std::string_view message) {
    std::ofstream logFile("resultError");
    logFile << file << ":" << line << " " << function << " " << condition << " :kek: " << message;
    logFile.close();
  });
  YACLIB_ERROR(1 != 2, "1 != 2");
  YACLIB_ERROR(1 == 2, "1 == 2");
  std::stringstream expected;
  expected << __FILE__ << ":" << 33 << " " << YACLIB_FUNCTION_NAME << " "
           << "1 != 2"
           << " :kek: "
           << "1 != 2";
  std::ifstream logFile("resultError");
  std::stringstream buffer;
  buffer << logFile.rdbuf();
  ASSERT_EQ(buffer.str(), expected.str());
}

TEST_F(LoggingTest, Info) {
  SetInfoCallback([](std::string_view file, std::size_t line, std::string_view function, std::string_view condition,
                     std::string_view message) {
    std::ofstream logFile("resultInfo");
    logFile << file << ":" << line << " " << function << " " << condition << " :kek: " << message;
    logFile.close();
  });
  YACLIB_INFO(1 != 2, "1 != 2");
  YACLIB_INFO(1 == 2, "1 == 2");
  std::stringstream expected;
  expected << __FILE__ << ":" << 53 << " " << YACLIB_FUNCTION_NAME << " "
           << "1 != 2"
           << " :kek: "
           << "1 != 2";
  std::ifstream logFile("resultInfo");
  std::stringstream buffer;
  buffer << logFile.rdbuf();
  ASSERT_EQ(buffer.str(), expected.str());
}

TEST_F(LoggingTest, WithoutCallback) {
  SetErrorCallback(nullptr);
  SetInfoCallback(nullptr);
  ASSERT_NO_FATAL_FAILURE({
    YACLIB_INFO(1 != 2, "1 != 2");
    YACLIB_ERROR(1 != 2, "1 != 2");
  });
}

TEST_F(LoggingTest, ResetCallback) {
  SetErrorCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                      std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "0";
    logFile.close();
  });
  YACLIB_ERROR(true, "0");
  SetErrorCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                      std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "1";
    logFile.close();
  });
  YACLIB_ERROR(true, "1");

  SetInfoCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                     std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "2";
    logFile.close();
  });
  YACLIB_INFO(true, "2");
  SetInfoCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                     std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "3";
    logFile.close();
  });
  YACLIB_INFO(true, "3");

  std::ifstream logFile("resultResetCallback");
  std::stringstream buffer;
  buffer << logFile.rdbuf();
  ASSERT_EQ(buffer.str(), "0123");
}

#include <yaclib/log.hpp>

#include <fstream>

#include <gtest/gtest.h>

class LoggingTest : public ::testing::Test {
 protected:
  void TearDown() override {
    yaclib::SetErrorCallback([](std::string_view file, std::size_t line, std::string_view /*function*/,
                                std::string_view /*condition*/, std::string_view message) {
      GTEST_MESSAGE_AT_(file.data(), line, message.data(), ::testing::TestPartResult::kFatalFailure);
    });
    yaclib::SetInfoCallback([](std::string_view file, std::size_t line, std::string_view function,
                               std::string_view /*condition*/, std::string_view message) {
      std::cerr << "[          ] [ INFO ] " << message << " in" << file << ":" << line
                << ". Function name: " << function;
    });
    std::remove("resultError");
    std::remove("resultInfo");
    std::remove("resultResetCallback");
  }
};

TEST_F(LoggingTest, Error) {
  yaclib::SetErrorCallback([](std::string_view file, std::size_t line, std::string_view function,
                              std::string_view condition, std::string_view message) {
    std::ofstream logFile("resultError");
    logFile << file << ":" << line << " " << function << " " << condition << " :kek: " << message;
    logFile.close();
  });
  YACLIB_ERROR(1 != 2, "1 != 2");
  YACLIB_ERROR(1 == 2, "1 == 2");
  std::stringstream expected;
  expected << __FILE__ << ":" << 32 << " " << YACLIB_FUNCTION_NAME << " "
           << "1 != 2"
           << " :kek: "
           << "1 != 2";
  std::ifstream logFile("resultError");
  std::stringstream buffer;
  buffer << logFile.rdbuf();
  ASSERT_EQ(buffer.str(), expected.str());
}

TEST_F(LoggingTest, Info) {
  yaclib::SetInfoCallback([](std::string_view file, std::size_t line, std::string_view function,
                             std::string_view condition, std::string_view message) {
    std::ofstream logFile("resultInfo");
    logFile << file << ":" << line << " " << function << " " << condition << " :kek: " << message;
    logFile.close();
  });
  YACLIB_INFO(1 != 2, "1 != 2");
  YACLIB_INFO(1 == 2, "1 == 2");
  std::stringstream expected;
  expected << __FILE__ << ":" << 52 << " " << YACLIB_FUNCTION_NAME << " "
           << "1 != 2"
           << " :kek: "
           << "1 != 2";
  std::ifstream logFile("resultInfo");
  std::stringstream buffer;
  buffer << logFile.rdbuf();
  ASSERT_EQ(buffer.str(), expected.str());
}

TEST_F(LoggingTest, WithoutCallback) {
  yaclib::SetErrorCallback(nullptr);
  yaclib::SetInfoCallback(nullptr);
  ASSERT_NO_FATAL_FAILURE({
    YACLIB_INFO(1 != 2, "1 != 2");
    YACLIB_ERROR(1 != 2, "1 != 2");
  });
}

TEST_F(LoggingTest, ResetCallback) {
  yaclib::SetErrorCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                              std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "0";
    logFile.close();
  });
  YACLIB_ERROR(true, "0");
  yaclib::SetErrorCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                              std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "1";
    logFile.close();
  });
  YACLIB_ERROR(true, "1");

  yaclib::SetInfoCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
                             std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultResetCallback", std::ios_base::app);
    logFile << "2";
    logFile.close();
  });
  YACLIB_INFO(true, "2");
  yaclib::SetInfoCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view /*function*/,
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

int testFuncName(int a) {
  YACLIB_ERROR(true, "kek");
  return 0;
}

TEST_F(LoggingTest, Function) {
  yaclib::SetErrorCallback([](std::string_view /*file*/, std::size_t /*line*/, std::string_view function,
                              std::string_view /*condition*/, std::string_view /*message*/) {
    std::ofstream logFile("resultFunction");
    logFile << function;
    logFile.close();
  });

  testFuncName(1);

  std::ifstream logFile("resultFunction");
  std::stringstream buffer;
  buffer << logFile.rdbuf();
  std::cerr << "[          ] [ INFO ] " << buffer.str() << '\n';
  ASSERT_TRUE(buffer.str().find("testFuncName") != std::string::npos);
}

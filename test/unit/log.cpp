#include <util/helpers.hpp>

#include <yaclib/log.hpp>

#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

namespace test {
namespace {

class LoggingTest : public ::testing::Test {
 protected:
  void TearDown() override {
    std::remove("log_file_error");
    std::remove("log_file_info");
    std::remove("log_file_debug");
    std::remove("log_file_shared");
    test::InitLog();
  }
};

std::string_view testFuncNameAbracadabra([[maybe_unused]] int kek) {
  return YACLIB_FUNC_NAME;
}

TEST_F(LoggingTest, Function) {
  auto name = testFuncNameAbracadabra(0);
  ASSERT_NE(name.find("testFuncNameAbracadabra"), name.npos);
  YACLIB_WARN(true, "Print testFuncNameAbracadabra function name");
}

constexpr bool kEverythingFine = false;
constexpr bool kSomeError = true;

TEST_F(LoggingTest, NullCallbacks) {
  YACLIB_INIT_WARN(nullptr);
  YACLIB_INIT_DEBUG(nullptr);
  ASSERT_NO_FATAL_FAILURE({
    YACLIB_WARN(kEverythingFine, "You use API incorrect");
    YACLIB_WARN(kSomeError, "You use API incorrect");
    YACLIB_DEBUG(kEverythingFine, "You use API incorrect");
    YACLIB_DEBUG(kSomeError, "You use API incorrect");
  });
}

TEST_F(LoggingTest, SepareteCallbacks) {
  auto info_callback = [](std::string_view file, std::size_t line, std::string_view function,
                          std::string_view condition, std::string_view message) noexcept {
    std::ofstream log_file{"log_file_info", std::ios_base::out | std::ios_base::app};
    log_file << "[ " << file << ":" << line << " in " << function << " ] Failed info condition: '" << condition
             << "' with message '" << message << "'\n";
  };
  auto debug_callback = [](std::string_view file, std::size_t line, std::string_view function,
                           std::string_view condition, std::string_view message) noexcept {
    std::ofstream log_file{"log_file_debug", std::ios_base::out | std::ios_base::app};
    log_file << "[ " << file << ":" << line << " in " << function << " ] Failed debug condition: '" << condition
             << "' with message '" << message << "'\n";
  };
  YACLIB_INIT_WARN(info_callback);
  YACLIB_INIT_DEBUG(debug_callback);
  std::stringstream expected_output_info;
  std::stringstream expected_output_debug;
  YACLIB_WARN(kEverythingFine, "You use API incorrect");
  YACLIB_WARN(kSomeError, "You use API incorrect");
  expected_output_info << "[ " << __FILE__ << ":" << __LINE__ - 1 << " in " << YACLIB_FUNC_NAME
                       << " ] Failed info condition: 'kSomeError' with message 'You use API incorrect'\n";
  YACLIB_DEBUG(kEverythingFine, "You use API incorrect");
  YACLIB_DEBUG(kSomeError, "You use API incorrect");
  expected_output_debug << "[ " << __FILE__ << ":" << __LINE__ - 1 << " in " << YACLIB_FUNC_NAME
                        << " ] Failed debug condition: 'kSomeError' with message 'You use API incorrect'\n";
  std::ifstream log_file_error{"log_file_error"};
  std::ifstream log_file_info{"log_file_info"};
  std::ifstream log_file_debug{"log_file_debug"};
  auto check = [](std::ifstream& log_file, std::stringstream& expected_output) {
    std::stringstream log_output;
    log_output << log_file.rdbuf();
    ASSERT_EQ(log_output.str(), expected_output.str());
  };
  check(log_file_info, expected_output_info);
  check(log_file_debug, expected_output_debug);
}

TEST_F(LoggingTest, SharedCallbacks) {
  yaclib::LogCallback callback = [](std::string_view file, std::size_t line, std::string_view function,
                                    std::string_view condition, std::string_view message) noexcept {
    std::ofstream log_file{"log_file_shared", std::ios_base::out | std::ios_base::app};
    log_file << "[ " << file << ":" << line << " in " << function << " ] Failed some condition: '" << condition
             << "' with message '" << message << "'\n";
  };
  YACLIB_INIT_WARN(callback);
  YACLIB_INIT_DEBUG(callback);
  std::stringstream expected_output_shared;
  YACLIB_WARN(kEverythingFine, "You use API incorrect");
  YACLIB_WARN(kSomeError, "You use API incorrect");
  expected_output_shared << "[ " << __FILE__ << ":" << __LINE__ - 1 << " in " << YACLIB_FUNC_NAME
                         << " ] Failed some condition: 'kSomeError' with message 'You use API incorrect'\n";
  YACLIB_DEBUG(kEverythingFine, "You use API incorrect");
  YACLIB_DEBUG(kSomeError, "You use API incorrect");
  expected_output_shared << "[ " << __FILE__ << ":" << __LINE__ - 1 << " in " << YACLIB_FUNC_NAME
                         << " ] Failed some condition: 'kSomeError' with message 'You use API incorrect'\n";
  std::ifstream log_file_shared{"log_file_shared"};
  auto check = [](std::stringstream& expected_output, std::ifstream& log_file) {
    std::stringstream log_output;
    log_output << log_file.rdbuf();
    ASSERT_EQ(log_output.str(), expected_output.str());
  };
  check(expected_output_shared, log_file_shared);
}

}  // namespace
}  // namespace test

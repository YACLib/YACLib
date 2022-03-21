#include <yaclib/fault/fault_config.hpp>
#include <yaclib/log.hpp>

#include <cstdio>
#include <iostream>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
#ifdef __GLIBCPP__
  std::fprintf(stderr, "libstdc++: %d\n", __GLIBCPP__);
#endif
#ifdef __GLIBCXX__
  std::fprintf(stderr, "libstdc++: %d\n", __GLIBCXX__);
#endif
#ifdef _LIBCPP_VERSION
  std::fprintf(stderr, "libc++: %d\n", _LIBCPP_VERSION);
#endif
  ::testing::InitGoogleTest(&argc, argv);
  yaclib::SetErrorCallback([](std::string_view file, std::size_t line, std::string_view /*function*/,
                              std::string_view /*condition*/, std::string_view message) {
    GTEST_MESSAGE_AT_(file.data(), line, message.data(), ::testing::TestPartResult::kFatalFailure);
  });
  yaclib::SetInfoCallback([](std::string_view file, std::size_t line, std::string_view function,
                             std::string_view /*condition*/, std::string_view message) {
    std::cerr << "[          ] [ INFO ] " << message << " in" << file << ":" << line << ":" << function << '\n';
  });
  yaclib::SetFrequency(8);
  yaclib::SetSleepTime(200);
  return RUN_ALL_TESTS();
}

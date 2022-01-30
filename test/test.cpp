#include <yaclib/fault/fault_config.hpp>
#include <yaclib/log_config.hpp>

#include <cstdio>

#include <gtest/gtest.h>

#define GTEST_COUT std::cerr << "[          ] [ INFO ]"

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
  SetErrorCallback([](std::string_view msg, std::string_view file, size_t line, std::string_view function) {
    GTEST_FATAL_FAILURE_(msg.data());
  });
  SetInfoCallback([](std::string_view msg, std::string_view file, size_t line, std::string_view function) {
    GTEST_COUT << msg << " in" << file << ":" << line << ". Function name: " << function;
  });
  SetFrequency(8u);
  SetSleepTime(200u);
  YACLIB_INFO(true, "check YACLIB_INFO");
  return RUN_ALL_TESTS();
}

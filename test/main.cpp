#include "yaclib/fault/log_config.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  SetLogFatalCallback([](std::string_view str) {
    GTEST_FATAL_FAILURE_(str.data());
  });
  SetLogErrorCallback([](std::string_view str) {
    GTEST_NONFATAL_FAILURE_(str.data());
  });
  return RUN_ALL_TESTS();
}

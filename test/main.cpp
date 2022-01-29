#include <yaclib/fault/detail/antagonist/yielder.hpp>
#include <yaclib/fault/log_config.hpp>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  SetLogFatalCallback([](std::string_view str) {
    GTEST_FATAL_FAILURE_(str.data());
  });
  SetLogErrorCallback([](std::string_view str) {
    GTEST_NONFATAL_FAILURE_(str.data());
  });
  yaclib::detail::Yielder::SetFrequency(10u);
  yaclib::detail::Yielder::SetSleepTime(250u);
  return RUN_ALL_TESTS();
}

#include <util/helpers.hpp>

#include <yaclib/fault/config.hpp>
#include <yaclib/fault/detail/inject_fault.hpp>
#include <yaclib/log.hpp>

#include <cstdio>

#include <gtest/gtest.h>

namespace test {

const int seed = 1239;

class MyTestListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const testing::TestInfo& /*info*/) override {
    yaclib::SetSeed(seed);
    std::cout << "\n-------------- state" << yaclib::detail::GetYielder()->GetState() << "--------------\n";
  }
};

void InitLog() noexcept {
  auto assert_callback = [](std::string_view file, std::size_t line, std::string_view /*function*/,
                            std::string_view /*condition*/, std::string_view message) {
    GTEST_MESSAGE_AT_(file.data(), line, message.data(), ::testing::TestPartResult::kFatalFailure);
  };
  YACLIB_INIT_ERROR(assert_callback);
  YACLIB_INIT_INFO(nullptr);
  YACLIB_INIT_DEBUG(assert_callback);
}

void InitFault() {
  yaclib::SetFaultFrequency(8);
  yaclib::SetFaultSleepTime(200);
}

namespace {

void PrintEnvInfo() {
#ifdef __GLIBCPP__
  std::fprintf(stderr, "libstdc++: %d\n", __GLIBCPP__);
#endif
#ifdef __GLIBCXX__
  std::fprintf(stderr, "libstdc++: %d\n", __GLIBCXX__);
#endif
#ifdef _LIBCPP_VERSION
  std::fprintf(stderr, "libc++: %d\n", _LIBCPP_VERSION);
#endif
}

}  // namespace
}  // namespace test

int main(int argc, char** argv) {
  test::PrintEnvInfo();
  test::InitLog();
  test::InitFault();
  testing::InitGoogleTest(&argc, argv);
  int result = 0;
#ifdef YACLIB_FIBER
  ::testing::UnitTest::GetInstance()->listeners().Append(new test::MyTestListener());
  yaclib_std::thread tests([&]() {
    result = RUN_ALL_TESTS();
  });
  tests.join();
#else
  result = RUN_ALL_TESTS();
#endif
  return result;
}

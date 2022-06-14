#include <util/helpers.hpp>

#include <yaclib/fault/config.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/log.hpp>

#if YACLIB_FAULT == 2
#  include <yaclib/fault/detail/fiber/scheduler.hpp>
#endif

#include <cstdio>
#include <random>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {

auto rand = std::random_device();

auto seed = rand();

class MyTestListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const testing::TestInfo& /*info*/) override {
    std::cerr << "current random count" << yaclib::fiber::GetFaultRandomCount() << "\n";
  }
  void OnTestIterationStart(const testing::UnitTest& unitTest, int i) override {
    seed = rand();
    std::cerr << "seed: " << test::seed << "\n";
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
  yaclib::SetFaultFrequency(2 + std::random_device().operator()() % 4);
  yaclib::SetFaultSleepTime(200);
  yaclib::fiber::SetFaultRandomListPick(10);
  yaclib::fiber::SetFaultTickLength(10);
  yaclib::fiber::SetStackSize(24);
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
#if YACLIB_FAULT == 2
  ::testing::UnitTest::GetInstance()->listeners().Append(new test::MyTestListener());
  std::cerr << "seed: " << test::seed << "\n";
  yaclib::fault::Scheduler scheduler;
  yaclib::fault::Scheduler::Set(&scheduler);
  yaclib_std::thread tests([&]() {
    result = RUN_ALL_TESTS();
  });
  tests.join();
  YACLIB_ERROR(scheduler.IsRunning(), "scheduler is still running when tests are finished");
#else
  result = RUN_ALL_TESTS();
#endif
#if YACLIB_FAULT != 0
  std::cerr << "injected count: " << yaclib::GetInjectedCount() << std::endl;
#endif
  return result;
}

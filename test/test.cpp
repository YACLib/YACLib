#include <util/helpers.hpp>

#include <yaclib/config.hpp>
#include <yaclib/fault/config.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/log.hpp>

#if YACLIB_FAULT == 2
#  include <yaclib/fault/detail/fiber/scheduler.hpp>
#  include <yaclib/fault/detail/fiber/thread.hpp>
#endif

#include <iostream>
#include <random>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {

static std::random_device rand;

static std::size_t seed = rand();

class MyTestListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const testing::TestInfo& /*info*/) override {
    std::cerr << "current random count: " << yaclib::fiber::GetFaultRandomCount() << "\n";
  }
  void OnTestIterationStart(const testing::UnitTest& /*unitTest*/, int /*i*/) override {
    seed = rand();
    std::cerr << "seed: " << test::seed << "\n";
  }
};

void InitLog() noexcept {
  auto assert_callback = [](std::string_view file, std::size_t line, std::string_view /*function*/,
                            std::string_view /*condition*/, std::string_view message) noexcept {
    GTEST_MESSAGE_AT_(file.data(), line, message.data(), ::testing::TestPartResult::kFatalFailure);
  };
  YACLIB_INIT_DEBUG(assert_callback);
  YACLIB_INIT_WARN(nullptr);
}

void InitFault() {
  yaclib::SetFaultFrequency(2 + std::random_device().operator()() % 4);
  yaclib::SetFaultSleepTime(200);
  yaclib::fiber::SetFaultRandomListPick(10);
  yaclib::fiber::SetFaultTickLength(10);
  yaclib::fiber::SetStackSize(24);
}

namespace {

static_assert(YACLIB_FINAL_SUSPEND_TRANSFER <= YACLIB_SYMMETRIC_TRANSFER);

#define YACLIB_STRINGIZE(X) YACLIB_DO_STRINGIZE(X)
#define YACLIB_DO_STRINGIZE(X) #X

/*
#define YACLIB_JOIN(X, Y) YACLIB_DO_JOIN(X, Y)
#define YACLIB_DO_JOIN(X, Y) YACLIB_DO_JOIN2(X, Y)
#define YACLIB_DO_JOIN2(X, Y) X##Y
*/

#if defined(__clang_version__)
#  define YACLIB_COMPILER "clang : " __clang_version__
#elif defined(_MSC_FULL_VER) && defined(_MSC_VER)
#  define YACLIB_COMPILER "ms visual c++ : " YACLIB_STRINGIZE(_MSC_FULL_VER) " or " YACLIB_STRINGIZE(_MSC_VER)
#elif defined(_MSC_FULL_VER)
#  define YACLIB_COMPILER "ms visual c++ : " YACLIB_STRINGIZE(_MSC_FULL_VER)
#elif defined(_MSC_VER)
#  define YACLIB_COMPILER "ms visual c++ : " YACLIB_STRINGIZE(_MSC_VER)
#elif defined(__VERSION__)
#  define YACLIB_COMPILER "gnu c++ : " __VERSION__
#else
#  define YACLIB_COMPILER "unknown"
#endif

#if defined(__GLIBCXX__)
#  define YACLIB_STDLIB "gnu libstdc++ : " YACLIB_STRINGIZE(__GLIBCXX__)
#elif defined(__GLIBCPP__)
#  define YACLIB_STDLIB "gnu libstdc++ : " YACLIB_STRINGIZE(__GLIBCPP__)
#elif defined(_LIBCPP_VERSION)
#  define YACLIB_STDLIB "libc++ : " YACLIB_STRINGIZE(_LIBCPP_VERSION)
#else
#  define YACLIB_STDLIB "unknown"
#endif

#if defined(_WIN64)
#  define YACLIB_PLATFORM "windows 64-bit"
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define YACLIB_PLATFORM "windows 32-bit"
#elif defined(__APPLE__)
#  define YACLIB_PLATFORM "apple"
#elif defined(__ANDROID__)
#  define YACLIB_PLATFORM "android"
#elif defined(__linux__)
#  define YACLIB_PLATFORM "linux"
#elif defined(__unix__)
#  define YACLIB_PLATFORM "unix"
#elif defined(_POSIX_VERSION)
#  define YACLIB_PLATFORM "posix"
#else
#  define YACLIB_PLATFORM "unknown"
#endif

void PrintEnvInfo() {
  std::cerr << "compiler: " << YACLIB_COMPILER << "\nstdlib: " << YACLIB_STDLIB "\nplatform: " << YACLIB_PLATFORM
            << std::endl;
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
  YACLIB_DEBUG(scheduler.IsRunning(), "scheduler is still running when tests are finished");
#else
  result = RUN_ALL_TESTS();
#endif
#if YACLIB_FAULT != 0
  std::cerr << "injected count: " << yaclib::GetInjectedCount() << std::endl;
#endif
  return result;
}

#pragma once

#include <chrono>
#include <cstdlib>
#include <ctime>

#include <gtest/gtest.h>

namespace test::util {

/* TODO(Ri7ay) dont work on windows, check thread_pool tests */
#if defined(GTEST_OS_LINUX) || defined(GTEST_OS_MAC)

class ProcessCPUTimer {
 public:
  ProcessCPUTimer() {
    Reset();
  }

  [[nodiscard]] std::chrono::microseconds Elapsed() const {
    return std::chrono::microseconds(ElapsedMicros());
  }

  void Reset() {
    _start_ts = std::clock();
  }

 private:
  [[nodiscard]] std::size_t ElapsedMicros() const {
    const auto clocks = static_cast<std::size_t>(std::clock() - _start_ts);
    return ClocksToMicros(clocks);
  }

  static std::size_t ClocksToMicros(const std::size_t clocks) {
    return (clocks * 1'000'000) / CLOCKS_PER_SEC;
  }

  std::clock_t _start_ts{};
};

class ThreadCPUTimer {
 public:
  ThreadCPUTimer() {
    Reset();
  }

  [[nodiscard]] std::chrono::nanoseconds Elapsed() const {
    return std::chrono::nanoseconds(ElapsedNanos());
  }

  void Reset() {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &_start);
  }

 private:
  [[nodiscard]] std::uint64_t ElapsedNanos() const {
    timespec now{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);

    return ToNanos(now) - ToNanos(_start);
  }

  static std::uint64_t ToNanos(const timespec& tp) {
    return static_cast<std::uint64_t>(tp.tv_sec * 1'000'000'000 + tp.tv_nsec);
  }

  timespec _start{};
};

#endif

}  // namespace test::util

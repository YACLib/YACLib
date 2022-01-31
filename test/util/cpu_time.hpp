#pragma once

#include <chrono>
#include <cstdlib>
#include <ctime>

namespace test::util {

/* TODO(Ri7ay): dont work on windows, check thread_pool tests */
#if __linux
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
  [[nodiscard]] size_t ElapsedMicros() const {
    const auto clocks = static_cast<size_t>(std::clock() - _start_ts);
    return ClocksToMicros(clocks);
  }

  static size_t ClocksToMicros(const size_t clocks) {
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
  [[nodiscard]] uint64_t ElapsedNanos() const {
    timespec now{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);

    return ToNanos(now) - ToNanos(_start);
  }

  static uint64_t ToNanos(const timespec& tp) {
    return static_cast<uint64_t>(tp.tv_sec * 1'000'000'000 + tp.tv_nsec);
  }

  timespec _start{};
};
#endif  // linux

}  // namespace test::util

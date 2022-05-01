#pragma once
#include <yaclib_std/chrono>

namespace test::util {

using Duration = std::chrono::nanoseconds;

template <typename Clock = yaclib_std::chrono::steady_clock>
class StopWatch {
  using TimePoint = typename Clock::time_point;

 public:
  StopWatch() : start_(Now()) {
    static_assert(Clock::is_steady, "Steady clock required");
  }

  Duration Elapsed() const {
    return Now() - start_;
  }

  Duration Restart() {
    auto elapsed = Elapsed();
    start_ = Now();
    return elapsed;
  }

  static TimePoint Now() {
    return Clock::now();
  }

 private:
  TimePoint start_;
};

}  // namespace test::util

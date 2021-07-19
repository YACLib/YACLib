#pragma once
#include <chrono>

namespace util {

using Duration = std::chrono::nanoseconds;

template <typename Clock = std::chrono::steady_clock>
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

 private:
  static TimePoint Now() {
    return Clock::now();
  }

 private:
  TimePoint start_;
};

}  // namespace util

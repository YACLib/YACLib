#pragma once
#include <chrono>

namespace test::util {

using Duration = std::chrono::nanoseconds;

template <typename Clock = std::chrono::steady_clock>
class StopWatch {
  using TimePoint = typename Clock::time_point;

 public:
  StopWatch() : _start(Now()) {
    static_assert(Clock::is_steady, "Steady clock required");
  }

  Duration Elapsed() const {
    return Now() - _start;
  }

  Duration Restart() {
    auto elapsed = Elapsed();
    _start = Now();
    return elapsed;
  }

  static TimePoint Now() {
    return Clock::now();
  }

 private:
  TimePoint _start;
};

}  // namespace test::util

#pragma once

#include <yaclib_std/chrono>
#include <yaclib_std/thread>

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

inline void Reschedule() {
  // TODO(MBkkt) deduplicate this code
#if YACLIB_FAULT == 2
  yaclib_std::this_thread::yield();
#elif defined(_MSC_VER)
  yaclib_std::this_thread::yield();
#else
  yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds{1});
#endif
}

}  // namespace test::util

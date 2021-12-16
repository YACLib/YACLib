#include <yaclib/fault_injection/chrono/system_clock.hpp>

namespace yaclib::std::chrono::detail {

SystemClock::time_point SystemClock::now() {
  return ::std::chrono::system_clock::now();
}

time_t SystemClock::to_time_t(const SystemClock::time_point& time_point) _NOEXCEPT {
  return ::std::chrono::system_clock::to_time_t(time_point);
}

SystemClock::time_point SystemClock::from_time_t(time_t c_time_point) _NOEXCEPT {
  return ::std::chrono::system_clock::from_time_t(c_time_point);
}

}  // namespace yaclib::std::chrono::detail

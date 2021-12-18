#include <yaclib/fault/detail/chrono/system_clock.hpp>

namespace yaclib::detail {

SystemClock::time_point SystemClock::now() {
  // TODO(myannyax)
  return SystemClock::time_point();
}

time_t SystemClock::to_time_t(const SystemClock::time_point& time_point) _NOEXCEPT {
  return std::chrono::system_clock::to_time_t(time_point);
}

SystemClock::time_point SystemClock::from_time_t(time_t c_time_point) _NOEXCEPT {
  return std::chrono::system_clock::from_time_t(c_time_point);
}

}  // namespace yaclib::detail

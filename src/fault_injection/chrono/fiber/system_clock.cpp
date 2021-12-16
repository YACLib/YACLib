#include "yaclib/fault_injection/chrono/system_clock.hpp"

namespace yaclib::fault_injection::chrono::detail {

system_clock::time_point system_clock::now() {
  // TODO(myannyax)
  return system_clock::time_point();
}

time_t system_clock::to_time_t(const system_clock::time_point& time_point) _NOEXCEPT {
  return std::chrono::system_clock::to_time_t(time_point);
}

system_clock::time_point system_clock::from_time_t(time_t c_time_point) _NOEXCEPT {
  return std::chrono::system_clock::from_time_t(c_time_point);
}

}  // namespace yaclib::fault_injection::chrono::detail

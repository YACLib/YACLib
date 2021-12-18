#pragma once

#include <chrono>

namespace yaclib::detail {

class SystemClock {
 public:
  using rep = ::std::chrono::system_clock::rep;
  using period = ::std::chrono::system_clock::period;
  using duration = ::std::chrono::system_clock::duration;
  using time_point = ::std::chrono::system_clock::time_point;

  const bool is_steady = false;

  static time_point now();

  static time_t to_time_t(const time_point& time_point) _NOEXCEPT;

  static time_point from_time_t(time_t c_time_point) _NOEXCEPT;
};

}  // namespace yaclib::detail

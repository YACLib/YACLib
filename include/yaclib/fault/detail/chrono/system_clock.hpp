#pragma once

#include <yaclib/config.hpp>

#include <chrono>

namespace yaclib::detail {

struct SystemClock {
  using rep = std::chrono::system_clock::rep;
  using period = std::chrono::system_clock::period;
  using duration = std::chrono::system_clock::duration;
  using time_point = std::chrono::system_clock::time_point;

  static constexpr bool is_steady = false;

  static time_point now();

  static time_t to_time_t(const time_point& time_point) noexcept;

  static time_point from_time_t(time_t c_time_point) noexcept;
};

}  // namespace yaclib::detail

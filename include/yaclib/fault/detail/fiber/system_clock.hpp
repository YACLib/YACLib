#pragma once

#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <chrono>

namespace yaclib::detail::fiber {

struct SystemClock {
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<SystemClock>;

  static constexpr bool is_steady = true;

  static time_point now();

  static time_t to_time_t(const time_point& time_point) noexcept;

  static time_point from_time_t(time_t c_time_point) noexcept;
};

}  // namespace yaclib::detail::fiber

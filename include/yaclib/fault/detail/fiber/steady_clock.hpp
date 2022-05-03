#pragma once

#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <chrono>

namespace yaclib::detail::fiber {

struct SteadyClock {
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<SteadyClock>;

  static constexpr bool is_steady = true;

  static time_point now();
};

}  // namespace yaclib::detail::fiber

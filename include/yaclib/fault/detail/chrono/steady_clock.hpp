#pragma once

#include <yaclib/config.hpp>

#include <chrono>

namespace yaclib::detail {

struct SteadyClock {
  using rep = std::chrono::steady_clock::rep;
  using period = std::chrono::steady_clock::period;
  using duration = std::chrono::steady_clock::duration;
  using time_point = std::chrono::steady_clock::time_point;

  static constexpr bool is_steady = true;

  static time_point now();
};

}  // namespace yaclib::detail

#pragma once

#include <chrono>

namespace yaclib::detail {

class SteadyClock {
 public:
  using rep = ::std::chrono::steady_clock::rep;
  using period = ::std::chrono::steady_clock::period;
  using duration = ::std::chrono::steady_clock::duration;
  using time_point = ::std::chrono::steady_clock::time_point;

  const bool is_steady = true;

  static time_point now();

 private:
};

}  // namespace yaclib::detail

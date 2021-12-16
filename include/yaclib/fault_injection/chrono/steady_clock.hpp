#pragma once

#include <chrono>

namespace yaclib::std::chrono {

#if defined(YACLIB_FAULTY)

namespace detail {
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
}  // namespace detail

using steady_clock = detail::SteadyClock;

#else

using steady_clock = ::std::chrono::steady_clock;

#endif

}  // namespace yaclib::std::chrono

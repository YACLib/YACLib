#include <yaclib/fault_injection/chrono/steady_clock.hpp>

namespace yaclib::std::chrono::detail {

SteadyClock::time_point SteadyClock::now() {
  return ::std::chrono::steady_clock::now();
}

}  // namespace yaclib::std::chrono::detail

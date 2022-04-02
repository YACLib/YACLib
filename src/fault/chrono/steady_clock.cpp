#include <yaclib/config.hpp>
#include <yaclib/fault/detail/chrono/steady_clock.hpp>

namespace yaclib::detail {

SteadyClock::time_point SteadyClock::now() {
  return std::chrono::steady_clock::now();
}

}  // namespace yaclib::detail

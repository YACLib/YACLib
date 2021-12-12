#include "steady_clock.hpp"

namespace yaclib::std::chrono {

SteadyClock::time_point SteadyClock::now() {
  return ::std::chrono::steady_clock::now();
}

}  // namespace yaclib::std::chrono

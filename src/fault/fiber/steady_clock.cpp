#include <yaclib/fault/detail/fiber/steady_clock.hpp>

namespace yaclib::detail::fiber {
SteadyClock::time_point SteadyClock::now() {
  return time_point{duration{fault::Scheduler::GetScheduler()->GetTimeNs()}};
}
}  // namespace yaclib::detail::fiber

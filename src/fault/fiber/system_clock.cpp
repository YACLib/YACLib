#include <yaclib/fault/detail/fiber/system_clock.hpp>

namespace yaclib::detail::fiber {

SystemClock::time_point SystemClock::now() {
  return time_point{duration{fault::Scheduler::GetScheduler()->GetTimeNs()}};
}

time_t SystemClock::to_time_t(const SystemClock::time_point& time_point) noexcept {
  return static_cast<std::time_t>(
    std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count());
}

SystemClock::time_point SystemClock::from_time_t(time_t c_time_point) noexcept {
  return std::chrono::time_point_cast<SystemClock::duration>(
    std::chrono::time_point<SystemClock, std::chrono::seconds>(std::chrono::seconds(c_time_point)));
}

}  // namespace yaclib::detail::fiber

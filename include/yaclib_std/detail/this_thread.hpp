#pragma once

#include <yaclib_std/chrono>

#if YACLIB_FAULT_THIS_THREAD == 2
#  include <yaclib/fault/detail/fiber/scheduler.hpp>

namespace yaclib_std::this_thread {

template <typename Clock, typename Duration>
inline void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time) {
  std::uint64_t timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(sleep_time.time_since_epoch()).count();
  yaclib::fault::Scheduler::GetScheduler()->Sleep(timeout);
}

template <typename Rep, typename Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
  sleep_until(yaclib_std::chrono::steady_clock::now() + sleep_duration);
}

inline constexpr auto* yield = &yaclib::fault::Scheduler::RescheduleCurrent;

inline constexpr auto* get_id = &yaclib::fault::Scheduler::GetId;

}  // namespace yaclib_std::this_thread
// #elif YACLIB_FAULT_THIS_THREAD == 1  // TODO(myannyax) Maybe implement
// #  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <thread>

namespace yaclib_std::this_thread {

using std::this_thread::sleep_for;

using std::this_thread::sleep_until;

using std::this_thread::yield;

using std::this_thread::get_id;

}  // namespace yaclib_std::this_thread
#endif

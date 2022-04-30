#pragma once

#include <yaclib/fault/chrono.hpp>

#ifdef YACLIB_FAULT

#  ifdef YACLIB_FIBER

#    include <yaclib/fault/detail/thread/fiber_threadlike.hpp>

namespace yaclib_std {

using thread = yaclib::detail::FiberThreadlike;

}  // namespace yaclib_std

#  else

#    include <yaclib/fault/detail/thread/thread.hpp>

namespace yaclib_std {

using thread = yaclib::detail::Thread;

}  // namespace yaclib_std

#  endif

#else

#  include <thread>

namespace yaclib_std {

using thread = std::thread;

}  // namespace yaclib_std

#endif

#ifdef YACLIB_FIBER

namespace yaclib_std::this_thread {

template <class Clock, class Duration>
inline void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time) {
  yaclib::detail::GetScheduler()->SleepUntil(sleep_time);
}

template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
  yaclib::detail::GetScheduler()->SleepFor(sleep_duration);
}

inline constexpr auto* yield = &yaclib::detail::Scheduler::RescheduleCurrent;

inline constexpr auto* get_id = &yaclib::detail::Scheduler::GetId;

}  // namespace yaclib_std::this_thread

#else

namespace yaclib_std::this_thread {

using std::this_thread::sleep_for;

using std::this_thread::sleep_until;

using std::this_thread::yield;

using std::this_thread::get_id;

}  // namespace yaclib_std::this_thread

#endif

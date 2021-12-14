#pragma once

#include <yaclib/fault_injection/chrono/steady_clock.hpp>
#include <yaclib/fault_injection/thread/thread.hpp>

namespace yaclib::std::this_thread {

inline void sleep_for(const ::std::chrono::nanoseconds& time);

template <class Duration>
inline void sleep_until(const ::std::chrono::time_point<yaclib::std::chrono::steady_clock, Duration>& time_point);

inline void yield() noexcept;

inline yaclib::std::thread::id get_id() noexcept;

}  // namespace yaclib::std::this_thread

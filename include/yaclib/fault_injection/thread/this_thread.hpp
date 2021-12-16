#pragma once

#include <yaclib/fault_injection/chrono/steady_clock.hpp>
#include <yaclib/fault_injection/thread/thread.hpp>

namespace yaclib::std::this_thread {

void sleep_for(const ::std::chrono::nanoseconds& time);

template <class Duration>
void sleep_until(const ::std::chrono::time_point<yaclib::std::chrono::steady_clock, Duration>& time_point);

void yield() noexcept;

yaclib::std::thread::id get_id() noexcept;

}  // namespace yaclib::std::this_thread

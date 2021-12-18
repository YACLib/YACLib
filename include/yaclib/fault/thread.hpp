#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/thread/thread.hpp>

#include <thread>

namespace yaclib::std {

#ifdef YACLIB_FAULTY

using thread = detail::Thread;

#else

using thread = ::std::thread;

#endif

namespace this_thread {

void sleep_for(const ::std::chrono::nanoseconds& time);

template <typename Duration>
void sleep_until(const ::std::chrono::time_point<yaclib::std::chrono::steady_clock, Duration>& time_point);

void yield() noexcept;

::yaclib::std::thread::id get_id() noexcept;

}  // namespace this_thread

}  // namespace yaclib::std

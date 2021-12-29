#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/thread/thread.hpp>

#include <thread>

namespace yaclib_std {

#ifdef YACLIB_FAULT

using thread = yaclib::detail::Thread;

#else

using thread = std::thread;

#endif

namespace this_thread {

using sleep_for_function_type = void (*)(const std::chrono::nanoseconds& time);

inline constexpr sleep_for_function_type sleep_for = std::this_thread::sleep_for;

template <typename Duration>
using sleep_until_function_type =
    void (*)(const std::chrono::time_point<yaclib_std::chrono::steady_clock, Duration>& time_point);

template <typename Duration>
inline constexpr sleep_until_function_type<Duration> sleep_until = std::this_thread::sleep_until;

using yield_function_type = void (*)();

inline constexpr yield_function_type yield = std::this_thread::yield;

using get_id_function_type = std::thread::id (*)();

inline constexpr get_id_function_type get_id = std::this_thread::get_id;

}  // namespace this_thread

}  // namespace yaclib_std

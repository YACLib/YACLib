#pragma once

#include <yaclib/fault/chrono.hpp>

#ifdef YACLIB_FAULT
#include <yaclib/fault/detail/thread/thread.hpp>
#else
#include <thread>
#endif

namespace yaclib_std {

#ifdef YACLIB_FIBER

using thread = yaclib::detail::Thread;

#else

using thread = std::thread;

#endif

}  // namespace yaclib_std

namespace yaclib_std::this_thread {

using std::this_thread::sleep_for;

using std::this_thread::sleep_until;

#ifndef YACLIB_FIBER

using std::this_thread::yield;

#endif

using std::this_thread::get_id;

}  // namespace yaclib_std::this_thread

#ifdef YACLIB_FIBER

namespace yaclib_std::this_thread {
inline constexpr auto* yield = &yaclib::Coroutine::Yield;
}  // namespace yaclib_std::this_thread

using std::this_thread::yield;

#endif

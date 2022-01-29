#pragma once

#include <yaclib/fault/chrono.hpp>

#ifdef YACLIB_FAULT

#include <yaclib/fault/detail/thread/thread.hpp>

namespace yaclib_std {

using thread = yaclib::detail::Thread;

#else

#include <thread>

namespace yaclib_std {

using thread = std::thread;

}  // namespace yaclib_std

#endif

namespace yaclib_std::this_thread {

using std::this_thread::sleep_for;

using std::this_thread::sleep_until;

using std::this_thread::yield;

using std::this_thread::get_id;

}  // namespace yaclib_std::this_thread

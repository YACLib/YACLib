#pragma once

#include <yaclib/fault/chrono.hpp>

#ifdef YACLIB_FAULT

#  include <yaclib/fault/detail/inject_fault.hpp>
#  include <yaclib/fault/detail/mutex/shared_mutex.hpp>
#  include <yaclib/fault/detail/mutex/shared_timed_mutex.hpp>

namespace yaclib_std {

using shared_mutex = yaclib::detail::SharedMutex;
using shared_timed_mutex = yaclib::detail::SharedTimedMutex;

}  // namespace yaclib_std

#else

#  include <shared_mutex>

namespace yaclib_std {

using shared_mutex = std::shared_mutex;
using shared_timed_mutex = std::shared_timed_mutex;

}  // namespace yaclib_std

#endif

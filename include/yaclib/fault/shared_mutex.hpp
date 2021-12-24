#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/antagonist/inject_fault.hpp>
#include <yaclib/fault/detail/mutex/shared_mutex.hpp>
#include <yaclib/fault/detail/mutex/shared_timed_mutex.hpp>

#include <shared_mutex>
#include <unordered_set>

namespace yaclib_std {

#ifdef YACLIB_FAULT

using shared_mutex = yaclib::detail::SharedMutex;
using shared_timed_mutex = yaclib::detail::SharedTimedMutex;

#else

using shared_mutex = std::shared_mutex;
using shared_timed_mutex = std::shared_timed_mutex;

#endif
}  // namespace yaclib_std

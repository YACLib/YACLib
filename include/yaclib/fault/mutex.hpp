#pragma once

#ifdef YACLIB_FAULT

#  ifdef YACLIB_FIBER

#    include <yaclib/fault/detail/mutex/fiber_mutex.hpp>

// TODO(myannyax) proper mutexes of other types
namespace yaclib_std {

using mutex = yaclib::detail::FiberMutex;
using timed_mutex = yaclib::detail::FiberMutex;

using recursive_timed_mutex = yaclib::detail::FiberMutex;
using recursive_mutex = yaclib::detail::FiberMutex;

}  // namespace yaclib_std

#  else

#    include <yaclib/fault/detail/mutex/mutex.hpp>
#    include <yaclib/fault/detail/mutex/recursive_mutex.hpp>
#    include <yaclib/fault/detail/mutex/recursive_timed_mutex.hpp>
#    include <yaclib/fault/detail/mutex/timed_mutex.hpp>

namespace yaclib_std {

using mutex = yaclib::detail::Mutex;
using timed_mutex = yaclib::detail::TimedMutex;

using recursive_timed_mutex = yaclib::detail::RecursiveTimedMutex;
using recursive_mutex = yaclib::detail::RecursiveMutex;

}  // namespace yaclib_std

#  endif

#else

#  include <mutex>

namespace yaclib_std {

using mutex = std::mutex;
using timed_mutex = std::timed_mutex;

using recursive_timed_mutex = std::recursive_timed_mutex;
using recursive_mutex = std::recursive_mutex;

}  // namespace yaclib_std

#endif

#pragma once

#if YACLIB_FAULT_RECURSIVE_TIMED_MUTEX == 2
#  include <yaclib/fault/detail/fiber/recursive_timed_mutex.hpp>
#  include <yaclib/fault/detail/recursive_timed_mutex.hpp>

namespace yaclib_std {

using recursive_timed_mutex = yaclib::detail::RecursiveTimedMutex<yaclib::detail::fiber::RecursiveTimedMutex>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_RECURSIVE_TIMED_MUTEX == 1
#  include <yaclib/fault/detail/recursive_timed_mutex.hpp>

#  include <mutex>

namespace yaclib_std {

using recursive_timed_mutex = yaclib::detail::RecursiveTimedMutex<std::recursive_timed_mutex>;

}  // namespace yaclib_std
#else
#  include <mutex>

namespace yaclib_std {

using recursive_timed_mutex = std::recursive_timed_mutex;

}  // namespace yaclib_std
#endif

#pragma once

#if YACLIB_FAULT_TIMED_MUTEX == 2

#  include <yaclib/fault/detail/fiber/timed_mutex.hpp>
#  include <yaclib/fault/detail/timed_mutex.hpp>

namespace yaclib_std {

using timed_mutex = yaclib::detail::TimedMutex<yaclib::detail::fiber::TimedMutex>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_TIMED_MUTEX == 1
#  include <yaclib/fault/detail/timed_mutex.hpp>

#  include <mutex>

namespace yaclib_std {

using timed_mutex = yaclib::detail::TimedMutex<std::timed_mutex>;

}  // namespace yaclib_std
#else
#  include <mutex>

namespace yaclib_std {

using timed_mutex = std::timed_mutex;

}  // namespace yaclib_std
#endif

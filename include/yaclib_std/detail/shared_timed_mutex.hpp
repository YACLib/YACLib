#pragma once

#if YACLIB_FAULT_SHARED_TIMED_MUTEX == 2

#  include <yaclib/fault/detail/fiber/shared_timed_mutex.hpp>
#  include <yaclib/fault/detail/shared_timed_mutex.hpp>

namespace yaclib_std {

using shared_timed_mutex = yaclib::detail::SharedTimedMutex<yaclib::detail::fiber::SharedTimedMutex>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_SHARED_TIMED_MUTEX == 1
#  include <yaclib/fault/detail/shared_timed_mutex.hpp>

#  include <shared_mutex>

namespace yaclib_std {

using shared_timed_mutex = yaclib::detail::SharedTimedMutex<std::shared_timed_mutex>;

}  // namespace yaclib_std
#else
#  include <shared_mutex>

namespace yaclib_std {

using shared_timed_mutex = std::shared_timed_mutex;

}  // namespace yaclib_std
#endif

#pragma once

#if YACLIB_FAULT_RECURSIVE_MUTEX == 2
#  include <yaclib/fault/detail/fiber/recursive_mutex.hpp>
#  include <yaclib/fault/detail/recursive_mutex.hpp>

namespace yaclib_std {

using recursive_mutex = yaclib::detail::RecursiveMutex<yaclib::detail::fiber::RecursiveMutex>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_RECURSIVE_MUTEX == 1
#  include <yaclib/fault/detail/recursive_mutex.hpp>

#  include <mutex>

namespace yaclib_std {

using recursive_mutex = yaclib::detail::RecursiveMutex<std::recursive_mutex>;

}  // namespace yaclib_std
#else
#  include <mutex>

namespace yaclib_std {

using recursive_mutex = std::recursive_mutex;

}  // namespace yaclib_std
#endif

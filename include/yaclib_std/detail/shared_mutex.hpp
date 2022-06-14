#pragma once

#if YACLIB_FAULT_SHARED_MUTEX == 2

#  include <yaclib/fault/detail/fiber/shared_mutex.hpp>
#  include <yaclib/fault/detail/shared_mutex.hpp>

namespace yaclib_std {

using shared_mutex = yaclib::detail::SharedMutex<yaclib::detail::fiber::SharedMutex>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_SHARED_MUTEX == 1
#  include <yaclib/fault/detail/shared_mutex.hpp>

#  include <shared_mutex>

namespace yaclib_std {

using shared_mutex = yaclib::detail::SharedMutex<std::shared_mutex>;

}  // namespace yaclib_std
#else
#  include <shared_mutex>

namespace yaclib_std {

using shared_mutex = std::shared_mutex;

}  // namespace yaclib_std
#endif

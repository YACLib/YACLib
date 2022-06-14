#pragma once

#if YACLIB_FAULT_MUTEX == 2
#  include <yaclib/fault/detail/fiber/mutex.hpp>
#  include <yaclib/fault/detail/mutex.hpp>

namespace yaclib_std {

using mutex = yaclib::detail::Mutex<yaclib::detail::fiber::Mutex>;

}  // namespace yaclib_std

#elif YACLIB_FAULT_MUTEX == 1
#  include <yaclib/fault/detail/mutex.hpp>

#  include <mutex>

namespace yaclib_std {

using mutex = yaclib::detail::Mutex<std::mutex>;

}  // namespace yaclib_std
#else
#  include <mutex>

namespace yaclib_std {

using mutex = std::mutex;

}  // namespace yaclib_std
#endif

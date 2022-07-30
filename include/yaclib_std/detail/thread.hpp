#pragma once

#if YACLIB_FAULT_THREAD == 2
#  include <yaclib/fault/detail/fiber/thread.hpp>

namespace yaclib_std {

using thread = yaclib::detail::fiber::Thread;

}  // namespace yaclib_std

// #elif YACLIB_FAULT_THREAD == 1  // TODO(myannyax) Maybe implement
// #  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <thread>

namespace yaclib_std {

using thread = std::thread;

}  // namespace yaclib_std
#endif

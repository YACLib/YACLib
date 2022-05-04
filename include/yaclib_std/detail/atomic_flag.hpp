#pragma once

#if YACLIB_FAULT_ATOMIC_FLAG == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"

#  include <yaclib/fault/detail/atomic_flag.hpp>
#  include <yaclib/fault/detail/fiber/atomic_flag.hpp>

namespace yaclib_std {

using atomic_flag = yaclib::detail::AtomicFlag<yaclib::detail::fiber::AtomicFlag>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_ATOMIC_FLAG == 1
#  include <yaclib/fault/detail/atomic_flag.hpp>

#  include <atomic>

namespace yaclib_std {

using atomic_flag = yaclib::detail::AtomicFlag<std::atomic_flag>;

}  // namespace yaclib_std
#else
#  include <atomic>

namespace yaclib_std {

using std::atomic_flag;

}  // namespace yaclib_std
#endif

#pragma once

#if YACLIB_FAULT_RANDOM_DEVICE == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"

#  include <yaclib/fault/detail/fiber/random_device.hpp>

namespace yaclib_std::random {

using random_device = yaclib::detail::fiber::RandomDevice;

}  // namespace yaclib_std::random
#elif YACLIB_FAULT_RANDOM_DEVICE == 1
#  include <yaclib/fault/detail/thread/random_device.hpp>

namespace yaclib_std::random {

using random_device = yaclib::detail::thread::RandomDevice;

}  // namespace yaclib_std::random
#else
#  include <random>

namespace yaclib_std::random {

using std::random_device;

}  // namespace yaclib_std::random
#endif

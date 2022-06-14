#pragma once

#if YACLIB_FAULT_RANDOM_DEVICE == 2
#  include <yaclib/fault/detail/random_device.hpp>

namespace yaclib_std::random {

using random_device = yaclib::detail::thread::RandomDevice;

}  // namespace yaclib_std::random
#else
#  include <random>

namespace yaclib_std::random {

using std::random_device;

}  // namespace yaclib_std::random
#endif

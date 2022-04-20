#pragma once

#ifdef YACLIB_FAULT

#  include <yaclib/fault/detail/random/random_device.hpp>

namespace yaclib_std::random {

using random_device = yaclib::detail::RandomDevice;

}  // namespace yaclib_std::random

#else

#  include <random>

namespace yaclib_std::random {

using std::random_device;

}  // namespace yaclib_std::random

#endif

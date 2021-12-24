#pragma once

#include <yaclib/fault/detail/random/random_device.hpp>

#include <random>
#include <string>

namespace yaclib_std::random {

#ifdef YACLIB_FAULT

using random_device = yaclib::detail::RandomDevice;

#else

using random_device = std::random::random_device;

#endif

}  // namespace yaclib_std::random

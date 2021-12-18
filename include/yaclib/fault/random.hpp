#pragma once

#include <yaclib/fault/detail/random/random_device.hpp>

#include <random>
#include <string>

namespace yaclib::std::random {

#if defined(YACLIB_FAULTY)

using random_device = detail::RandomDevice;

#else

using random_device = ::std::random::random_device;

#endif

}  // namespace yaclib::std::random

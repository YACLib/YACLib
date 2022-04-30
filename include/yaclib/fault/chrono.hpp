#pragma once

#ifdef YACLIB_FIBER
#  include <yaclib/fault/detail/chrono/fiber_system_clock.hpp>

namespace yaclib_std::chrono {

using steady_clock = yaclib::detail::SystemClock;

using high_resolution_clock = yaclib::detail::SystemClock;

using system_clock = yaclib::detail::SystemClock;

}  // namespace yaclib_std::chrono
#else

#  include <chrono>

namespace yaclib_std::chrono {

using steady_clock = std::chrono::steady_clock;

using high_resolution_clock = std::chrono::high_resolution_clock;

using system_clock = std::chrono::system_clock;

}  // namespace yaclib_std::chrono
#endif

#pragma once

#include <yaclib/fault/detail/chrono/steady_clock.hpp>
#include <yaclib/fault/detail/chrono/system_clock.hpp>

namespace yaclib::std::chrono {

#if defined(YACLIB_FAULTY)

using steady_clock = yaclib::detail::SteadyClock;

using high_resolution_clock = yaclib::detail::SystemClock;

using system_clock = yaclib::detail::SystemClock;

#else

using steady_clock = ::std::chrono::steady_clock;

using high_resolution_clock = ::std::chrono::high_resolution_clock;

using system_clock = ::std::chrono::system_clock;

#endif

}  // namespace yaclib::std::chrono

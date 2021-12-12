#pragma once
#include "system_clock.hpp"

#include <ctime>

namespace yaclib::std::chrono {

#if defined(YACLIB_FAULTY)

::std::clock_t clock();

using high_resolution_clock = system_clock;

#else

::std::clock_t clock();

using high_resolution_clock = ::std::chrono::high_resolution_clock;

#endif

}  // namespace yaclib::std::chrono

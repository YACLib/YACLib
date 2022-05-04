#pragma once

#if YACLIB_FAULT_CLOCK == 2  // TODO(myannyax) Implement
#  error "YACLIB_FAULT=FIBER not implemented yet"
#elif YACLIB_FAULT_CLOCK == 1  // TODO(myannyax) Implement model time for thread
#  error "YACLIB_FAULT=THREAD not implemented yet"
#else
#  include <chrono>

namespace yaclib_std::chrono {

using system_clock = std::chrono::system_clock;
using steady_clock = std::chrono::steady_clock;
using high_resolution_clock = std::chrono::high_resolution_clock;

// TODO(myannyax) Implement, needs ifdef because these from C++20
// using utc_clock = std::chrono::utc_clock;
// using tai_clock = std::chrono::tai_clock;
// using gps_clock = std::chrono::gps_clock;
// using file_clock = std::chrono::file_clock;

}  // namespace yaclib_std::chrono
#endif

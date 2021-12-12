#include "other.hpp"

#if defined(YACLIB_FAULTY)

::std::clock_t yaclib::std::chrono::clock() {
  return system_clock::to_time_t(system_clock::now());
}

#else

::std::clock_t yaclib::fault_injection::chrono::clock() {
  return std::clock();
}

#endif

#include <yaclib/fault/fault_config.hpp>

void SetFrequency(uint32_t freq) {
#ifdef YACLIB_FAULT
  yaclib::detail::Yielder::SetFrequency(freq);
#endif
}

void SetSleepTime(uint32_t ns) {
#ifdef YACLIB_FAULT
  yaclib::detail::Yielder::SetSleepTime(ns);
#endif
}

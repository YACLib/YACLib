#include <yaclib/fault/fault_config.hpp>

#ifdef YACLIB_FAULT
#  include "util.hpp"

#  include <yaclib/fault/detail/yielder.hpp>
#endif

namespace yaclib {

void SetFrequency([[maybe_unused]] std::uint32_t freq) {
#ifdef YACLIB_FAULT
  yaclib::detail::Yielder::SetFrequency(freq);
#endif
}

void SetSleepTime([[maybe_unused]] std::uint32_t ns) {
#ifdef YACLIB_FAULT
  yaclib::detail::Yielder::SetSleepTime(ns);
#endif
}

void SetSeed(std::uint32_t seed) {
#ifdef YACLIB_FAULT
  detail::SetSeed(seed);
#endif
}

}  // namespace yaclib

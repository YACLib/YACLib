#include <yaclib/fault/config.hpp>
#include <yaclib/fault/injector.hpp>

namespace yaclib {

void SetFaultFrequency(std::uint32_t freq) {
  yaclib::detail::Injector::SetFrequency(freq);
}

void SetFaultSleepTime(std::uint32_t ns) {
  yaclib::detail::Injector::SetSleepTime(ns);
}

}  // namespace yaclib

#include "fault/util.hpp"

#include <yaclib/fault/detail/thread/random_device.hpp>

namespace yaclib::detail::thread {

RandomDevice::RandomDevice() : _eng{GetSeed()} {
}

RandomDevice::result_type RandomDevice::operator()() noexcept {
  return _eng();
}

double RandomDevice::entropy() const noexcept {
  return kEntropy;
}

constexpr RandomDevice::result_type RandomDevice::min() {
  return kMin;
}

constexpr RandomDevice::result_type RandomDevice::max() {
  return kMax;
}

void RandomDevice::reset() {
  // not thread safe
  _eng.seed(GetSeed());
}

}  // namespace yaclib::detail::thread

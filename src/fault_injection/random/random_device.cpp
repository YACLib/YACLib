//
// Created by Maria.Filipanova on 12/12/21.
//

#include <yaclib/fault_injection/random/random_device.hpp>

namespace yaclib::std::random {

RandomDevice::RandomDevice() : _eng(kSeed) {
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
  _eng.seed(kSeed);
}

}  // namespace yaclib::std::random

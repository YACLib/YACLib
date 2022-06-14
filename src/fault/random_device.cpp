#include <fault/util.hpp>

#include <yaclib/fault/detail/random_device.hpp>

namespace yaclib::detail::thread {

RandomDevice::RandomDevice() : _eng{GetSeed()} {
}

RandomDevice::result_type RandomDevice::operator()() noexcept {
  return _eng();
}

double RandomDevice::entropy() const noexcept {
  return 0;
}

constexpr RandomDevice::result_type RandomDevice::min() {
  return std::mt19937_64::min();
}

constexpr RandomDevice::result_type RandomDevice::max() {
  return std::mt19937_64::max();
}

void RandomDevice::reset() {
  _eng.seed(GetSeed());
}

RandomDevice::RandomDevice(const std::string&) : RandomDevice() {
}

}  // namespace yaclib::detail::thread

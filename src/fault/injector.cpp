#include <yaclib/fault/injector.hpp>

#include <yaclib_std/thread>

namespace yaclib::detail {

inline constexpr int kFreq = 16;
inline constexpr int kSleepTimeNs = 200;

std::atomic_uint32_t Injector::yield_frequency = kFreq;
std::atomic_uint32_t Injector::sleep_time = kSleepTimeNs;

// TODO(myannyax) maybe scheduler-wide random engine?
Injector::Injector() : _eng(1142), _count(0) {
}

void Injector::MaybeInject() {
  if (NeedInject()) {
    yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds(RandNumber(sleep_time)));
  }
}

bool Injector::NeedInject() {
  if (_count += 1 >= yield_frequency) {
    Reset();
    return true;
  }
  return false;
}

void Injector::Reset() {
  _count = RandNumber(yield_frequency);
}

uint32_t Injector::RandNumber(uint32_t max) {
  return 1 + _eng() % (max - 1);
}

void Injector::SetFrequency(uint32_t freq) {
  yield_frequency.store(freq);
}

void Injector::SetSleepTime(uint32_t ns) {
  sleep_time.store(ns);
}

}  // namespace yaclib::detail

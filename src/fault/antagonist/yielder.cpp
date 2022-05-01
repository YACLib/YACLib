#include "fault/util.hpp"

#include <yaclib/fault/detail/yielder.hpp>

namespace yaclib::detail {

inline constexpr int kFreq = 16;
inline constexpr int kSleepTimeNs = 200;

std::atomic_uint32_t Yielder::yield_frequency = kFreq;
std::atomic_uint32_t Yielder::sleep_time = kSleepTimeNs;

// TODO(myannyax) maybe scheduler-wide random engine?
Yielder::Yielder() : _count(0) {
}

void Yielder::MaybeYield() {
  if (ShouldYield()) {
#ifdef YACLIB_FIBER
    yaclib_std::this_thread::yield();
#else
    yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds(1 + GetRandNumber(sleep_time - 1)));
#endif
  }
}

bool Yielder::ShouldYield() {
  if (_count++ >= yield_frequency) {
    Reset();
    return true;
  }
  return false;
}

void Yielder::Reset() {
  _count = 1 + GetRandNumber(yield_frequency - 1);
}

void Yielder::SetFrequency(uint32_t freq) {
  yield_frequency.store(freq);
}

void Yielder::SetSleepTime(uint32_t ns) {
  sleep_time.store(ns);
}

void Yielder::SetState(uint32_t state) {
  _count = state;
}

uint32_t Yielder::GetState() {
  return _count;
}

}  // namespace yaclib::detail

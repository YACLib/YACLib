#include <fault/util.hpp>

#include <yaclib/fault/injector.hpp>

#include <yaclib_std/thread>

namespace yaclib::detail {

inline constexpr int kFreq = 16;
inline constexpr int kSleepTimeNs = 200;

std::atomic_uint32_t Injector::yield_frequency = kFreq;
std::atomic_uint32_t Injector::sleep_time = kSleepTimeNs;
std::atomic_uint64_t Injector::injected_count = 0;

Injector::Injector() : _count{0} {
}

void Injector::MaybeInject() {
  if (NeedInject()) {
#if YACLIB_FAULT == 2
    yaclib_std::this_thread::yield();
#else
    yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds(1 + GetRandNumber(sleep_time)));
#endif
    injected_count.fetch_add(1);
  }
}

bool Injector::NeedInject() {
  if (_pause) {
    return false;
  }
  if (++_count >= yield_frequency) {
    Reset();
    return true;
  }
  return false;
}

void Injector::Reset() {
  _count = GetRandNumber(yield_frequency);
}

void Injector::SetFrequency(uint32_t freq) {
  yield_frequency.store(freq);
}

void Injector::SetSleepTime(uint32_t ns) {
  sleep_time.store(ns);
}

uint64_t Injector::GetInjectedCount() {
  return injected_count.load();
}

uint32_t Injector::GetState() const {
  return _count;
}

void Injector::SetState(uint32_t state) {
  _count = state;
}

void Injector::SetPauseInject(bool pause) {
  _pause = pause;
}

}  // namespace yaclib::detail

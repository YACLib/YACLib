#include <yaclib/fault/detail/antagonist/yielder.hpp>

#define YACLIB_FAULT_SLEEP_TIME_NS 200

namespace yaclib::detail {

std::atomic_uint32_t Yielder::yield_frequency = kFreq;
std::atomic_uint32_t Yielder::sleep_time = YACLIB_FAULT_SLEEP_TIME_NS;

// TODO(myannyax) maybe scheduler-wide random engine?
Yielder::Yielder() : _eng(1142) {
}

void Yielder::MaybeYield() {
  if (ShouldYield()) {
    yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds(RandNumber(sleep_time)));
  }
}

bool Yielder::ShouldYield() {
  if (_count.fetch_add(1, std::memory_order_acq_rel) >= yield_frequency) {
    Reset();
    return true;
  }
  return false;
}

void Yielder::Reset() {
  _count.exchange(RandNumber(yield_frequency));
}

unsigned Yielder::RandNumber(uint32_t max) {
  return 1 + _eng() % (max - 1);
}

void Yielder::SetFrequency(uint32_t freq) {
  yield_frequency.store(freq);
}

void Yielder::SetSleepTime(uint32_t ns) {
  sleep_time.store(sleep_time);
}

}  // namespace yaclib::detail

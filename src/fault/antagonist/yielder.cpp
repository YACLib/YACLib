#include <yaclib/fault/detail/antagonist/yielder.hpp>

namespace yaclib::detail {

// TODO(myannyax) maybe scheduler-wide random engine?
Yielder::Yielder(int frequency) : _eng(1142), _freq(frequency) {
}

void Yielder::MaybeYield() {
  if (ShouldYield()) {
    std::this_thread::sleep_for(::std::chrono::milliseconds(RandNumber(SLEEP_TIME)));
  }
}

bool Yielder::ShouldYield() {
  if (_count.fetch_add(1) >= _freq) {
    Reset();
    return true;
  }
  return false;
}

void Yielder::Reset() {
  _count.exchange(RandNumber(_freq));
}

unsigned Yielder::RandNumber(unsigned max) {
  return 1 + _eng() % (max - 1);
}

}  // namespace yaclib::detail

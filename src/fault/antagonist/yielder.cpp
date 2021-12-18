#include <yaclib/fault/detail/antagonist/yielder.hpp>

namespace yaclib::detail {

// TODO(myannyax) maybe scheduler-wide random engine?
Yielder::Yielder(int frequency) : _eng(1142), _freq(frequency) {
}

void Yielder::MaybeYield() {
  if (ShouldYield()) {
    std::this_thread::yield();
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
  auto step = 1 + _eng() % (_freq - 1);
  _count.exchange(step);
}

}  // namespace yaclib::detail

#include "yielder.hpp"

namespace yaclib::std {

void Yielder::MaybeYield() {
  if (ShouldYield()) {
    this_thread::yield();
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
  auto step = _eng() % _freq;
  _count.exchange(step);
}

}  // namespace yaclib::std

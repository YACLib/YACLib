#include <yaclib/util/detail/atomic_event.hpp>

namespace yaclib::detail {

AtomicEvent::Token AtomicEvent::Make() noexcept {
  return {};
}

void AtomicEvent::Wait(Token) noexcept {
  _state.wait(0, std::memory_order_relaxed);
  while (_state.load(std::memory_order_acquire) != 2) {
  }
}

void AtomicEvent::SetOne() noexcept {
  _state.store(1, std::memory_order_relaxed);
  _state.notify_one();
  _state.store(2, std::memory_order_release);
}

void AtomicEvent::SetAll() noexcept {
  _state.store(1, std::memory_order_relaxed);
  _state.notify_all();
  _state.store(2, std::memory_order_release);
}

void AtomicEvent::Reset() noexcept {
  _state.store(0, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

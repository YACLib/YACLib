#include <yaclib/config.hpp>
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

void AtomicEvent::Reset(Token) noexcept {
  _state.store(0, std::memory_order_relaxed);
}

void AtomicEvent::Set() noexcept {
  _state.store(1, std::memory_order_relaxed);
  _state.notify_one();
  _state.store(2, std::memory_order_release);
}

}  // namespace yaclib::detail

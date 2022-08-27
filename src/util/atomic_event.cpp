#include <yaclib/util/detail/atomic_event.hpp>

namespace yaclib::detail {

AtomicEvent::Token AtomicEvent::Make() noexcept {
  return {};
}

void AtomicEvent::Wait(Token) noexcept {
#if YACLIB_FUTEX == 1
  _state.wait(0, std::memory_order_relaxed);
  while (_state.load(std::memory_order_acquire) != 2) {
  }
#elif YACLIB_FUTEX == 2
  _state.wait(0, std::memory_order_acquire);
#endif
}

void AtomicEvent::Set() noexcept {
#if YACLIB_FUTEX == 1
  _state.store(1, std::memory_order_relaxed);
  _state.notify_one();
  _state.store(2, std::memory_order_release);
#elif YACLIB_FUTEX == 2
  _state.store(1, std::memory_order_release);
  _state.notify_one();
#endif
}

void AtomicEvent::Reset() noexcept {
  _state.store(0, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

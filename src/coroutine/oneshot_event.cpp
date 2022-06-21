#include "yaclib/coroutine/oneshot_event.hpp"

namespace {}  // namespace

namespace yaclib {

OneShotEventOperation::OneShotEventOperation(IExecutor& executor, OneShotEvent& event) noexcept
    : _event{event}, _next{nullptr}, _core{nullptr}, _executor{executor} {
}

bool OneShotEventOperation::await_ready() const noexcept {
  return _event._head.load(std::memory_order_acquire) == OneShotEvent::kAllDone;
}

void OneShotEventOperation::await_resume() const noexcept {
}

OneShotEvent::OneShotEvent() noexcept : _head(OneShotEvent::kEmpty) {
}

void OneShotEvent::Set() noexcept {
  std::uintptr_t old_head = _head.exchange(OneShotEvent::kAllDone, std::memory_order_acq_rel);
  while (old_head != OneShotEvent::kEmpty) {
    OneShotEventOperation* node = reinterpret_cast<OneShotEventOperation*>(old_head);
    old_head = reinterpret_cast<std::uintptr_t>(node->_next);
    node->_executor.Submit(*node->_core);
  }
}

void OneShotEvent::Reset() noexcept {
  _head = OneShotEvent::kEmpty;
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(std::memory_order_relaxed) == OneShotEvent::kAllDone;
}

bool OneShotEvent::TryAdd(OneShotEventOperation* node) noexcept {
  std::uintptr_t old_head = _head.load(std::memory_order_acquire);
  std::uintptr_t core = reinterpret_cast<std::uintptr_t>(node);
  while (old_head != OneShotEvent::kAllDone) {
    node->_next = reinterpret_cast<OneShotEventOperation*>(old_head);
    if (_head.compare_exchange_weak(old_head, core, std::memory_order_release, std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

OneShotEventOperation OneShotEvent::Wait(IExecutor& executor) {
  return OneShotEventOperation{executor, *this};
}

}  // namespace yaclib

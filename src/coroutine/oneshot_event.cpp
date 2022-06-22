#include "yaclib/coroutine/oneshot_event.hpp"

namespace yaclib {

OneShotEventNode::OneShotEventNode(OneShotEventNode* next) : _next{next} {
}

OneShotEventOperation::OneShotEventOperation(IExecutor& executor, OneShotEvent& event) noexcept
    : OneShotEventNode{nullptr}, _event{event}, _core{nullptr}, _executor{executor} {
}

bool OneShotEventOperation::await_ready() const noexcept {
  return _event._head.load(std::memory_order_acquire) == OneShotEvent::kAllDone;
}

void OneShotEventOperation::await_resume() const noexcept {
}

void OneShotEventOperation::Process() noexcept {
  _executor.Submit(*_core);
}

OneShotEventWait::OneShotEventWait(OneShotEvent& event) noexcept : OneShotEventNode{nullptr}, _event{event} {
}

void OneShotEventWait::Process() noexcept {
  {
    std::lock_guard lk(_mutex);
    _done = true;
  }
  _cv.notify_all();
}

OneShotEvent::OneShotEvent() noexcept : _head{OneShotEvent::kEmpty} {
}

void OneShotEvent::Set() noexcept {
  std::uintptr_t old_head = _head.exchange(OneShotEvent::kAllDone, std::memory_order_acq_rel);
  while (old_head != OneShotEvent::kEmpty) {
    OneShotEventNode* node = reinterpret_cast<OneShotEventOperation*>(old_head);
    old_head = reinterpret_cast<std::uintptr_t>(node->_next);
    node->Process();
  }
}

void OneShotEvent::Reset() noexcept {
  _head.store(OneShotEvent::kEmpty, std::memory_order_release);
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(std::memory_order_relaxed) == OneShotEvent::kAllDone;
}

bool OneShotEvent::TryAdd(OneShotEventNode* node) noexcept {
  std::uintptr_t old_head = _head.load(std::memory_order_acquire);
  std::uintptr_t core = reinterpret_cast<std::uintptr_t>(node);
  while (old_head != OneShotEvent::kAllDone) {
    node->_next = reinterpret_cast<OneShotEventNode*>(old_head);
    if (_head.compare_exchange_weak(old_head, core, std::memory_order_release, std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

OneShotEventOperation OneShotEvent::Await(IExecutor& executor) {
  return OneShotEventOperation{executor, *this};
}

void OneShotEvent::Wait() {
  OneShotEventWait waiter{*this};
  if (TryAdd(&waiter)) {
    std::unique_lock lk{waiter._mutex};
    waiter._cv.wait(lk, [&]() {
      return waiter._done;
    });
  }
}

YACLIB_INLINE void Wait(OneShotEvent& event) {
  event.Wait();
}
}  // namespace yaclib

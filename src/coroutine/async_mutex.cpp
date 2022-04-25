#include <yaclib/coroutine/async_mutex.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/atomic.hpp>

#include <iostream>  // for debug

namespace yaclib {

AsyncMutex::AsyncMutex() : _state(NotLocked()), _waiters(nullptr) {
}

LockAwaiter AsyncMutex::Lock() {
  return LockAwaiter{*this};
}

// todo rename
void AsyncMutex::SimpleUnlock() {
  YACLIB_ERROR(_state.load(std::memory_order_relaxed) != NotLocked(), "unlock must be called after lock!");
  auto* head = _waiters;
  if (head == nullptr) {  // lock, no waiters
    auto old_state = static_cast<void*>(head);

    if (_state.compare_exchange_strong(old_state, NotLocked(), std::memory_order_release, std::memory_order_relaxed)) {
      return;
    }
    // now we have one more waiter

    old_state = _state.exchange(nullptr, std::memory_order_acquire);
    // old_state - linked stack of all mutex' waiters; now state is 'lock & no waiters', p.g. cleared

    YACLIB_DEBUG(old_state != nullptr && old_state != NotLocked(), "There must be awaiters!");

    // reverse
    auto* next = reinterpret_cast<Job*>(old_state);
    do {
      auto* temp = static_cast<Job*>(next->next);
      next->next = head;
      head = next;
      next = temp;
    } while (next != nullptr);
  }
  YACLIB_DEBUG(head != nullptr, "Must be locked with at least 1 awaiter!");

  _waiters = static_cast<Job*>(head->next);
  head->Call();  // inline resume
}

LockAwaiter::LockAwaiter(AsyncMutex& mutex) : _mutex(mutex) {
}

bool LockAwaiter::await_ready() noexcept {
  void* old_state = _mutex._state.load(std::memory_order_acquire);
  if (old_state != _mutex.NotLocked()) {
    return false;  // mutex is locked by another exec. thread
  }
  bool b =
    _mutex._state.compare_exchange_strong(old_state, nullptr, std::memory_order_acquire, std::memory_order_relaxed);
  return b;
}

}  // namespace yaclib

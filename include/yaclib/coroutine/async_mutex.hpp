#pragma once

#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/atomic.hpp>

#include <iostream>

namespace yaclib {

class AsyncMutex;
class LockAwaiter;

class AsyncMutex {
 public:
  AsyncMutex();
  LockAwaiter Lock();
  // todo rename
  void SimpleUnlock();

 private:
  friend class LockAwaiter;
  void* NotLocked() {
    return this;
  }

  // class UnlockAwaiter {
  //  public:
  //   explicit UnlockAwaiter(AsyncMutex& mutex) : _mutex(mutex) {
  //   }

  //  private:
  //   AsyncMutex& _mutex;
  // };

  yaclib_std::atomic<void*> _state;  // nullptr - lock, no waiters; this - no lock, otherwise - head of the list
  Job* _waiters;
};

class LockAwaiter {
 public:
  explicit LockAwaiter(AsyncMutex& mutex);

  bool await_ready() noexcept;
  void await_resume() {
  }

  template <class V, class E>
  auto await_suspend(yaclib_std::coroutine_handle<detail::PromiseType<V, E>> handle) {
    Job* promise_ptr = &handle.promise();
    void* old_state = _mutex._state.load(std::memory_order_acquire);
    while (true) {
      if (old_state == _mutex.NotLocked()) {
        if (_mutex._state.compare_exchange_weak(old_state, nullptr, std::memory_order_acquire,
                                                std::memory_order_relaxed)) {
          return false;  // continue
        }
      } else {
        // try to push on stack
        promise_ptr->next = reinterpret_cast<Job*>(old_state);
        if (_mutex._state.compare_exchange_weak(old_state, reinterpret_cast<void*>(promise_ptr),
                                                std::memory_order_release, std::memory_order_relaxed)) {
          return true;  // push on top of the stack
        }
      }
    }
  }

 private:
  AsyncMutex& _mutex;
};

}  // namespace yaclib

#pragma once

#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <yaclib_std/atomic>

namespace yaclib {

// TODO(mkornaukhov03) Doxygen docs

template <bool FIFO = false>
class AsyncMutex {
 public:
  AsyncMutex() noexcept = default;
  AsyncMutex(const AsyncMutex&) = delete;
  AsyncMutex(AsyncMutex&&) = delete;
  AsyncMutex& operator=(const AsyncMutex&) = delete;
  AsyncMutex& operator=(AsyncMutex&&) = delete;

  auto Guard() noexcept {
    return GuardAwaiter{*this};
  }

  auto TryGuard() noexcept {
    return LockGuard{*this, std::try_to_lock_t{}};
  }

  auto Lock() noexcept {
    return LockAwaiter{*this};
  }

  [[nodiscard]] bool TryLock() noexcept {
    void* old_state = _state.load(std::memory_order_acquire);
    if (old_state != NotLocked()) {
      return false;
    }
    return _state.compare_exchange_strong(old_state, LockedNoWaiters(), std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  enum class UnlockType {
    Auto,
    On,
  };

  template <UnlockType Type = UnlockType::Auto>
  auto Unlock(IExecutor& executor = CurrentThreadPool()) noexcept {
    return UnlockAwaiter<Type>{*this, executor};
  }

  void UnlockHere(IExecutor& executor = CurrentThreadPool()) noexcept {
    YACLIB_DEBUG(_state.load(std::memory_order_relaxed) == NotLocked(), "UnlockHere must be called after Lock!");
    auto* next = TryUnlock<FIFO>();
    if (next == nullptr) {
      return;
    }
    _next_cs_here = true;
    _waiters = static_cast<detail::BaseCore*>(next->next);
    executor.Submit(*next);
  }

  class [[nodiscard]] LockGuard {
   public:
    explicit LockGuard(AsyncMutex& m, std::defer_lock_t) noexcept : _mutex{&m}, _owns{false} {
    }
    explicit LockGuard(AsyncMutex& m, std::try_to_lock_t) noexcept : _mutex{&m}, _owns{m.TryLock()} {
    }
    explicit LockGuard(AsyncMutex& m, std::adopt_lock_t) noexcept : _mutex{&m}, _owns{true} {
    }

    [[nodiscard]] AsyncMutex* Mutex() const noexcept {
      return _mutex;
    }

    [[nodiscard]] bool OwnsLock() const noexcept {
      return _owns;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
      return OwnsLock();
    }

    auto Lock() noexcept {
      YACLIB_ERROR(_owns, "Cannot lock already locked mutex");
      _owns = true;
      return _mutex->Lock();
    }

    template <UnlockType Type = UnlockType::Auto>
    auto Unlock(IExecutor& executor = CurrentThreadPool()) {
      YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
      _owns = false;
      return _mutex->Unlock<Type>(executor);
    }

    void UnlockHere(IExecutor& executor = CurrentThreadPool()/*default value should exist only if we don't have co_await Unlock, so don't have symmetric transfer*/) noexcept {
      YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
      _owns = false;
      _mutex->UnlockHere(executor);
    }

    void Swap(AsyncMutex& oth) noexcept {
      std::swap(_mutex, oth._mutex);
      std::swap(_owns, oth._owns);
    }

    AsyncMutex* Release() noexcept {
      _owns = false;
      return _mutex;
    }

    ~LockGuard() noexcept {
      if (_owns) {
        YACLIB_INFO(true, "Better use co_await Guard::Unlock<UnlockType>(executor)");
        _mutex->UnlockHere(CurrentThreadPool());
      }
    }

   private:
    AsyncMutex* _mutex;
    bool _owns;  // TODO add as _mutex bit
  };

 private:
  class [[nodiscard]] LockAwaiter {
   public:
    explicit LockAwaiter(AsyncMutex& mutex) : _mutex{mutex} {
    }

    YACLIB_INLINE bool await_ready() noexcept {
      return _mutex.TryLock();
    }

    template <typename V, typename E>
    bool await_suspend(yaclib_std::coroutine_handle<detail::PromiseType<V, E>> handle) {
      detail::BaseCore& base_core = handle.promise();
      void* old_state = _mutex._state.load(std::memory_order_acquire);
      while (true) {
        if (old_state == _mutex.NotLocked()) {
          if (_mutex._state.compare_exchange_weak(old_state, _mutex.LockedNoWaiters(), std::memory_order_acquire,
                                                  std::memory_order_relaxed)) {
            return false;
          }
        } else {
          base_core.next = static_cast<detail::BaseCore*>(old_state);
          if (_mutex._state.compare_exchange_weak(old_state, &base_core, std::memory_order_release,
                                                  std::memory_order_relaxed)) {
            return true;
          }
        }
      }
    }

    YACLIB_INLINE void await_resume() noexcept {
    }

   protected:
    AsyncMutex& _mutex;
  };

  class [[nodiscard]] GuardAwaiter : public LockAwaiter {
   public:
    explicit GuardAwaiter(AsyncMutex& mutex) : LockAwaiter{mutex} {
    }

    YACLIB_INLINE LockGuard await_resume() noexcept {
      return LockGuard{LockAwaiter::_mutex, std::adopt_lock};
    }
  };

  template <UnlockType Type>
  class [[nodiscard]] UnlockAwaiter {
   public:
    explicit UnlockAwaiter(AsyncMutex& m, IExecutor& e) : _mutex{m}, _executor{e} {
    }

    bool await_ready() noexcept {
      if constexpr (Type == UnlockType::Auto) {
        auto* next = _mutex.TryUnlock<FIFO>();
        if (next == nullptr) {
          return true;
        }
        _mutex._waiters = next;
      }
      return false;
    }

    template <typename V, typename E>
    auto await_suspend(yaclib_std::coroutine_handle<detail::PromiseType<V, E>> handle) {
      detail::BaseCore* next;
      if constexpr (Type == UnlockType::Auto) {
        next = _mutex._waiters;
      } else {
        next = _mutex.TryUnlock<FIFO>();
        if (next == _mutex.LockedNoWaiters()) {
          _executor.Submit(handle.promise());
          YACLIB_SUSPEND();
          // YACLIB_RESUME(handle);
        }
      }
      _mutex._waiters = static_cast<detail::BaseCore*>(next->next);
      if (_mutex._next_cs_here) {
        _executor.Submit(handle.promise());
        YACLIB_TRANSFER(next->GetHandle());
      }
      _mutex._next_cs_here = true;
      _executor.Submit(*next);
      if constexpr (Type == UnlockType::Auto) {
        YACLIB_RESUME(handle);
      } else {
        _executor.Submit(handle.promise());
        YACLIB_SUSPEND();
        // YACLIB_RESUME(handle);
      }
    }

    YACLIB_INLINE void await_resume() noexcept {
    }

   private:
    AsyncMutex& _mutex;
    IExecutor& _executor;
  };

  template <bool UNLOCK_FIFO = FIFO>
  detail::BaseCore* TryUnlock() noexcept {
    if (_waiters != nullptr) {
      return _waiters;
    }
    const bool old_next_cs_here = std::exchange(_next_cs_here, false);
    void* old_state = LockedNoWaiters();
    if (_state.compare_exchange_strong(old_state, NotLocked(), std::memory_order_release, std::memory_order_relaxed)) {
      return nullptr;
    }
    old_state = _state.exchange(LockedNoWaiters(), std::memory_order_acquire);
    _next_cs_here = !old_next_cs_here;
    if constexpr (UNLOCK_FIFO) {
      detail::Node* node = static_cast<detail::BaseCore*>(old_state);
      detail::Node* prev = nullptr;
      do {
        auto* next = node->next;
        node->next = prev;
        prev = node;
        node = next;
      } while (node != nullptr);
      return static_cast<detail::BaseCore*>(prev);
    } else {
      return static_cast<detail::BaseCore*>(old_state);
    }
  }

  YACLIB_INLINE void* NotLocked() noexcept {
    return this;
  }

  constexpr void* LockedNoWaiters() noexcept {
    return nullptr;
  }

  yaclib_std::atomic<void*> _state =
    NotLocked();  // locked without waiters, not locked, otherwise - head of the awaiters list
  detail::BaseCore* _waiters = nullptr;
  bool _next_cs_here = false;  // TODO add as _waiters bit (suppose it won't give significant effect)
};

}  // namespace yaclib

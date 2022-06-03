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
    auto old_state = _state.load(std::memory_order_acquire);
    if (old_state != kNotLocked) {
      return false;
    }
    return _state.compare_exchange_strong(old_state, kLockedNoWaiters, std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  auto Unlock(IExecutor& executor = CurrentThreadPool()) noexcept {
    return UnlockAwaiter<UnlockType::Auto>{*this, executor};
  }

  auto UnlockOn(IExecutor& executor = CurrentThreadPool()) noexcept {
    return UnlockAwaiter<UnlockType::On>{*this, executor};
  }

  void UnlockHere(IExecutor& executor = CurrentThreadPool()) noexcept {
    YACLIB_DEBUG(_state.load(std::memory_order_relaxed) == kNotLocked, "UnlockHere must be called after Lock!");
    auto* next = TryUnlock<FIFO>();
    if (next == nullptr) {
      return;
    }
    _count_batch_here = 0;
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

    auto Unlock(IExecutor& executor = CurrentThreadPool()) noexcept {
      YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
      _owns = false;
      return _mutex->Unlock(executor);
    }

    auto UnlockOn(IExecutor& executor = CurrentThreadPool()) noexcept {
      YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
      _owns = false;
      return _mutex->UnlockOn(executor);
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
  enum class UnlockType {
    Auto,
    On,
  };

  class [[nodiscard]] LockAwaiter {
   public:
    explicit LockAwaiter(AsyncMutex& mutex) noexcept : _mutex{mutex} {
    }

    YACLIB_INLINE bool await_ready() noexcept {
      return _mutex.TryLock();
    }

    template <typename V, typename E>
    bool await_suspend(yaclib_std::coroutine_handle<detail::PromiseType<V, E>> handle) noexcept {
      detail::BaseCore& base_core = handle.promise();
      auto old_state = _mutex._state.load(std::memory_order_relaxed);
      while (true) {
        if (old_state == _mutex.kNotLocked) {
          if (_mutex._state.compare_exchange_weak(old_state, _mutex.kLockedNoWaiters, std::memory_order_acquire,
                                                  std::memory_order_relaxed)) {
            return false;
          }
        } else {
          base_core.next = reinterpret_cast<detail::BaseCore*>(old_state);
          if (_mutex._state.compare_exchange_weak(old_state, reinterpret_cast<std::uintptr_t>(&base_core),
                                                  std::memory_order_release, std::memory_order_relaxed)) {
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

  class [[nodiscard]] GuardAwaiter final : public LockAwaiter {
   public:
    explicit GuardAwaiter(AsyncMutex& mutex) noexcept : LockAwaiter{mutex} {
    }

    YACLIB_INLINE LockGuard await_resume() noexcept {
      return LockGuard{LockAwaiter::_mutex, std::adopt_lock};
    }
  };

  template <UnlockType Type>
  class [[nodiscard]] UnlockAwaiter final {
   public:
    explicit UnlockAwaiter(AsyncMutex& m, IExecutor& e) noexcept : _mutex{m}, _executor{e} {
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
    auto await_suspend(yaclib_std::coroutine_handle<detail::PromiseType<V, E>> handle) noexcept {
      detail::BaseCore* next;
      if constexpr (Type == UnlockType::Auto) {
        next = _mutex._waiters;
      } else {
        next = _mutex.TryUnlock<FIFO>();
        if (next == nullptr) {
          _executor.Submit(handle.promise());
          YACLIB_SUSPEND();
        }
      }
      _mutex._waiters = static_cast<detail::BaseCore*>(next->next);
      if (_mutex._count_batch_here <= 1) {
        _executor.Submit(handle.promise());
        YACLIB_TRANSFER(next->GetHandle());
      }
      _mutex._count_batch_here = 0;
      _executor.Submit(*next);
      if constexpr (Type == UnlockType::Auto) {
        YACLIB_RESUME(handle);
      } else {
        _executor.Submit(handle.promise());
        YACLIB_SUSPEND();
      }
    }

    YACLIB_INLINE void await_resume() noexcept {
    }

   private:
    AsyncMutex& _mutex;
    IExecutor& _executor;
  };

  template <bool UnlockFIFO = FIFO>
  detail::BaseCore* TryUnlock() noexcept {
    if (_waiters != nullptr) {
      return _waiters;
    }
    const auto old_count_batch_here = std::exchange(_count_batch_here, 0);
    auto old_state = kLockedNoWaiters;
    if (_state.compare_exchange_strong(old_state, kNotLocked, std::memory_order_release, std::memory_order_relaxed)) {
      return nullptr;
    }

    old_state = _state.exchange(kLockedNoWaiters, std::memory_order_acquire);
    _count_batch_here = old_count_batch_here + 1;
    if constexpr (UnlockFIFO) {
      detail::Node* node = reinterpret_cast<detail::BaseCore*>(old_state);
      detail::Node* prev = nullptr;
      do {
        auto* next = node->next;
        node->next = prev;
        prev = node;
        node = next;
      } while (node != nullptr);
      return static_cast<detail::BaseCore*>(prev);
    } else {
      return reinterpret_cast<detail::BaseCore*>(old_state);
    }
  }

  static constexpr std::uintptr_t kNotLocked = 1;
  static constexpr std::uintptr_t kLockedNoWaiters = 0;
  // locked without waiters, not locked, otherwise - head of the awaiters list
  yaclib_std::atomic<std::uintptr_t> _state = kNotLocked;  // TODO(mkornaukhov03)
  detail::BaseCore* _waiters = nullptr;
  std::size_t _count_batch_here = 0;
};

}  // namespace yaclib

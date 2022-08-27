#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/exe/strand.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <yaclib_std/atomic>

namespace yaclib {

// TODO(mkornaukhov03) Doxygen docs

template <bool DefaultFIFO = false, std::int64_t DefaultBatchHere = 1>
class AsyncMutex final {
 public:
  /**
   *
   */
  static constexpr std::int64_t kOn = -1;

  /**
   *
   */
  static constexpr std::int64_t kAll = std::numeric_limits<uint32_t>::max();

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

  template <bool FIFO = DefaultFIFO, std::int64_t BatchHere = DefaultBatchHere>
  auto Unlock(IExecutor& e = CurrentThreadPool()) noexcept {
    return UnlockAwaiter<FIFO, BatchHere>{*this, e};
  }

  template <bool FIFO = DefaultFIFO, std::int64_t BatchHere = DefaultBatchHere>
  auto UnlockOn(IExecutor& e = CurrentThreadPool()) noexcept {
    return Unlock<FIFO, kOn * BatchHere>(e);
  }

  template <bool FIFO = DefaultFIFO>
  void UnlockHere(IExecutor& e = CurrentThreadPool()) noexcept {
    YACLIB_DEBUG(_state.load(std::memory_order_relaxed) == kNotLocked, "UnlockHere must be called after Lock!");
    auto* next = TryUnlock<FIFO>();
    if (next == nullptr) {
      return;
    }
    _count_batch_here = 0;
    _waiters = static_cast<detail::BaseCore*>(next->next);
    e.Submit(*next);
  }

  class [[nodiscard]] LockGuard final {
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

    template <bool FIFO = DefaultFIFO, std::int64_t BatchHere = DefaultBatchHere>
    auto Unlock(IExecutor& e = CurrentThreadPool()) noexcept {
      YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
      _owns = false;
      return _mutex->Unlock<FIFO, BatchHere>(e);
    }

    template <bool FIFO = DefaultFIFO, std::int64_t BatchHere = DefaultBatchHere>
    auto UnlockOn(IExecutor& e = CurrentThreadPool()) noexcept {
      return Unlock<FIFO, kOn * BatchHere>(e);
    }

    template <bool FIFO = DefaultFIFO>
    void UnlockHere(IExecutor& e = CurrentThreadPool()) noexcept {
      YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
      _owns = false;
      _mutex->UnlockHere<FIFO>(e);
    }

    void Swap(AsyncMutex& other) noexcept {
      std::swap(_mutex, other._mutex);
      std::swap(_owns, other._owns);
    }

    AsyncMutex* Release() noexcept {
      _owns = false;
      return _mutex;
    }

    ~LockGuard() noexcept {
      if (_owns) {
        YACLIB_WARN(true, "Better use co_await LockGuard::Unlock(executor)");
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
    explicit LockAwaiter(AsyncMutex& mutex) noexcept : _mutex{mutex} {
    }

    YACLIB_INLINE bool await_ready() noexcept {
      return _mutex.TryLock();
    }

    template <typename Promise>
    bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      detail::BaseCore& base_core = handle.promise();
      auto old_state = _mutex._state.load(std::memory_order_relaxed);
      while (true) {
        if (old_state == AsyncMutex::kNotLocked) {
          if (_mutex._state.compare_exchange_weak(old_state, AsyncMutex::kLockedNoWaiters, std::memory_order_acquire,
                                                  std::memory_order_relaxed)) {
            return false;
          }
        } else {
          base_core.next = reinterpret_cast<detail::BaseCore*>(old_state);
          if (_mutex._state.compare_exchange_weak(old_state, reinterpret_cast<std::uint64_t>(&base_core),
                                                  std::memory_order_release, std::memory_order_relaxed)) {
            return true;
          }
        }
      }
    }

    constexpr void await_resume() noexcept {
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

  template <bool FIFO, std::int64_t BatchHere>
  class [[nodiscard]] UnlockAwaiter final {
   public:
    explicit UnlockAwaiter(AsyncMutex& m, IExecutor& e) noexcept : _mutex{m}, _executor{e} {
    }

    bool await_ready() noexcept {
      if constexpr (BatchHere >= 0) {
        auto* next = _mutex.TryUnlock<FIFO>();
        if (next == nullptr) {
          return true;
        }
        _mutex._waiters = next;
      }
      return false;
    }

    template <typename Promise>
    auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      detail::BaseCore* next;
      if constexpr (BatchHere >= 0) {
        next = _mutex._waiters;
      } else {
        next = _mutex.TryUnlock<FIFO>();
        if (next == nullptr) {
          _executor.Submit(handle.promise());
          YACLIB_SUSPEND();
        }
      }
      _mutex._waiters = static_cast<detail::BaseCore*>(next->next);
      if (_mutex._count_batch_here <= (BatchHere >= 0 ? BatchHere : -BatchHere)) {
        _executor.Submit(handle.promise());
        YACLIB_TRANSFER(next->Next());
      }
      _mutex._count_batch_here = 0;
      _executor.Submit(*next);
      if constexpr (BatchHere >= 0) {
        YACLIB_RESUME(handle);
      } else {
        _executor.Submit(handle.promise());
        YACLIB_SUSPEND();
      }
    }

    constexpr void await_resume() noexcept {
    }

   private:
    AsyncMutex& _mutex;
    IExecutor& _executor;
  };

  template <bool FIFO>
  detail::BaseCore* TryUnlock() noexcept {
    if (_waiters != nullptr) {
      return _waiters;
    }
    const auto old_count_batch_here = std::exchange(_count_batch_here, 0);
    auto old_state = kLockedNoWaiters;
    if (_state.load(std::memory_order_relaxed) == old_state &&
        _state.compare_exchange_strong(old_state, kNotLocked, std::memory_order_release, std::memory_order_relaxed)) {
      return nullptr;
    }

    old_state = _state.exchange(kLockedNoWaiters, std::memory_order_acquire);
    _count_batch_here = old_count_batch_here + 1;
    if constexpr (FIFO) {
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

  static constexpr std::uint64_t kNotLocked = 1;
  static constexpr std::uint64_t kLockedNoWaiters = 0;
  // locked without waiters, not locked, otherwise - head of the awaiters list
  yaclib_std::atomic_uint64_t _state = kNotLocked;  // TODO(mkornaukhov03)
  detail::BaseCore* _waiters = nullptr;
  std::uint32_t _count_batch_here = 0;
};

}  // namespace yaclib

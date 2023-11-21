#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/mutex_awaiter.hpp>
#include <yaclib/coro/detail/on_awaiter.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/coro/guard_sticky.hpp>

#include <yaclib_std/atomic>

namespace yaclib {
namespace detail {

template <bool FIFO, bool Batching>
struct MutexImpl {
  [[nodiscard]] bool TryLockAwait() noexcept {
    auto expected = kNotLocked;
    return _sender.load(std::memory_order_relaxed) == expected &&
           _sender.compare_exchange_strong(expected, kLockedNoWaiters, std::memory_order_acquire,
                                           std::memory_order_relaxed);
  }

  [[nodiscard]] bool AwaitLock(BaseCore& curr) noexcept {
    auto expected = _sender.load(std::memory_order_relaxed);
    while (true) {
      if (expected == kNotLocked) {
        if (_sender.compare_exchange_weak(expected, kLockedNoWaiters, std::memory_order_acquire,
                                          std::memory_order_relaxed)) {
          return false;
        }
      } else {
        curr.next = reinterpret_cast<BaseCore*>(expected);
        if (_sender.compare_exchange_weak(expected, reinterpret_cast<std::uintptr_t>(&curr), std::memory_order_release,
                                          std::memory_order_relaxed)) {
          return true;
        }
      }
    }
  }

  [[nodiscard]] bool TryUnlockAwait() noexcept {
    YACLIB_ASSERT(_sender.load(std::memory_order_relaxed) != kNotLocked);
    if (_receiver != nullptr) {
      return false;
    }
    auto expected = kLockedNoWaiters;
    return _sender.load(std::memory_order_relaxed) == expected &&
           _sender.compare_exchange_strong(expected, kNotLocked, std::memory_order_release, std::memory_order_relaxed);
  }

  [[nodiscard]] YACLIB_INLINE bool BatchingPossible() const noexcept {
    return Batching && _receiver != nullptr;
  }

  void UnlockHereAwait() noexcept {
    auto& next = GetHead();
    _receiver = static_cast<detail::BaseCore*>(next.next);
    // next._executor for next critical section
    next._executor->Submit(next);
  }

  [[nodiscard]] auto AwaitUnlock(BaseCore& curr) noexcept {
    YACLIB_ASSERT(_receiver != nullptr);
    auto& next = *_receiver;
    _receiver = static_cast<BaseCore*>(next.next);
    // curr._executor for next critical section
    // next._executor for current coroutine resume
    curr._executor.Swap(next._executor);
    curr._executor->Submit(curr);
    YACLIB_TRANSFER(next.Curr());
  }

  [[nodiscard]] auto AwaitUnlockOn(BaseCore& curr, IExecutor& executor) noexcept {
    // executor for current coroutine resume
    auto curr_executor = std::exchange(curr._executor, &executor);
    YACLIB_ASSERT(curr_executor != nullptr);
    executor.Submit(curr);
    if (TryUnlockAwait()) {
      YACLIB_SUSPEND();
    }
    auto& next = GetHead();
    if constexpr (Batching) {
      if (_receiver != nullptr) {
        _receiver = static_cast<BaseCore*>(next.next);
        // curr_executor for next critical section
        next._executor = std::move(curr_executor);
        YACLIB_TRANSFER(next.Curr());
      }
    }
    _receiver = static_cast<BaseCore*>(next.next);
    // next._executor for next critical section
    next._executor->Submit(next);
    YACLIB_SUSPEND();
  }

  [[nodiscard]] bool TryLock() noexcept {
    return TryLockAwait();
  }

  void UnlockHere() noexcept {
    if (!TryUnlockAwait()) {
      UnlockHereAwait();
    }
  }

 private:
  [[nodiscard]] BaseCore& GetHead() noexcept {
    if (_receiver != nullptr) {
      return *_receiver;
    }
    auto expected = _sender.exchange(kLockedNoWaiters, std::memory_order_acquire);
    if constexpr (FIFO) {
      Node* node = reinterpret_cast<BaseCore*>(expected);
      Node* prev = nullptr;
      do {
        auto* next = node->next;
        node->next = prev;
        prev = node;
        node = next;
      } while (node != nullptr);
      return *static_cast<BaseCore*>(prev);
    } else {
      return *reinterpret_cast<BaseCore*>(expected);
    }
  }

  static constexpr auto kLockedNoWaiters = std::uintptr_t{0};
  static constexpr auto kNotLocked = std::numeric_limits<std::uintptr_t>::max();

  // locked without waiters, not locked, otherwise - head of the waiters list
  yaclib_std::atomic_uintptr_t _sender = kNotLocked;
  BaseCore* _receiver = nullptr;
};

template <typename M>
class [[nodiscard]] UnlockAwaiter final {
 public:
  explicit UnlockAwaiter(M& m) noexcept : _mutex{m} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    if (_mutex.TryUnlockAwait()) {
      return true;
    }
    if (_mutex.BatchingPossible()) {
      return false;
    }
    _mutex.UnlockHereAwait();
    return true;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _mutex.AwaitUnlock(handle.promise());
  }

  constexpr void await_resume() noexcept {
  }

 private:
  M& _mutex;
};

template <typename M>
class [[nodiscard]] UnlockOnAwaiter final {
 public:
  explicit UnlockOnAwaiter(M& m, IExecutor& e) noexcept : _mutex{m}, _executor{e} {
  }

  constexpr bool await_ready() noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _mutex.AwaitUnlockOn(handle.promise(), _executor);
  }

  constexpr void await_resume() noexcept {
  }

 private:
  M& _mutex;
  IExecutor& _executor;
};

}  // namespace detail

/**
 * Mutex for coroutines
 *
 * \note It does not block execution thread, only coroutine
 */
template <bool Batching = true, bool FIFO = false>
class Mutex final : protected detail::MutexImpl<FIFO, Batching> {
 public:
  using Base = detail::MutexImpl<FIFO, Batching>;

  /**
   * Try to lock mutex and create UniqueGuard for it
   *
   * \note If we couldn't lock mutex, then UniqueGuard::OwnsLock() will be false
   * \return Awaitable, which await_resume returns the UniqueGuard
   */
  auto TryGuard() noexcept {
    return UniqueGuard<Mutex>{*this, std::try_to_lock};
  }

  /**
   * Lock mutex and create UniqueGuard for it
   *
   * \return Awaitable, which await_resume returns the UniqueGuard
   */
  auto Guard() noexcept {
    return detail::GuardAwaiter<UniqueGuard, Mutex>{*this};
  }

  /**
   * Lock mutex and create StickyGuard for it
   *
   * \return Awaitable, which await_resume returns the StickyGuard
   */
  auto GuardSticky() noexcept {
    return detail::GuardStickyAwaiter{*this};
  }

  /**
   * Try to lock mutex
   * return true if mutex was locked, false otherwise
   */
  using Base::TryLock;

  /**
   * Lock mutex
   */
  auto Lock() noexcept {
    return detail::LockAwaiter<Base>{*this};
  }

  /**
   * The best way to unlock mutex, if you interested in batched critical section
   */
  auto Unlock() noexcept {
    return detail::UnlockAwaiter<Base>{*this};
  }

  /**
   * This method is an optimization for Unlock() and On()
   *
   * Use it instead of
   * \code
   * ...
   * co_await mutex.Unlock();
   * co_await On(e);
   * ...
   * \endcode
   *
   * Typical usage:
   * \code
   * ...
   * // auto& executorBeforeCriticalSection = yaclib::kCurrent;
   * auto guard = co_await mutex.StickyGuard();
   * ...
   * co_await guard.UnlockOn();
   * // auto& executorAfterCriticalSection = yaclib::kCurrent;
   * // assert(&executorBeforeCriticalSection == &executorAfterCriticalSection);
   * ...
   * \endcode
   *
   * \param e executor which will be used for code after unlock
   */
  auto UnlockOn(IExecutor& e) noexcept {
    return detail::UnlockOnAwaiter<Base>{*this, e};
  }

  /**
   * The general way to unlock mutex, mainly for RAII
   */
  using Base::UnlockHere;

  // Helper for Awaiter implementation
  // TODO(MBkkt) get rid of it?
  template <typename To, typename From>
  static auto& Cast(From& from) noexcept {
    return static_cast<To&>(from);
  }
};

}  // namespace yaclib

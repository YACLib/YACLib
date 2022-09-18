#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/on_awaiter.hpp>
#include <yaclib/coro/detail/promise_type.hpp>

#include <yaclib_std/atomic>

namespace yaclib {
namespace detail {

struct MutexImpl {
  [[nodiscard]] bool TryLock() noexcept {
    auto expected = kNotLocked;
    if (sender.load(std::memory_order_relaxed) != expected) {
      return false;
    }
    return sender.compare_exchange_strong(expected, kLockedNoWaiters, std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  [[nodiscard]] bool Lock(BaseCore& new_state) noexcept {
    auto expected = sender.load(std::memory_order_relaxed);
    while (true) {
      if (expected == kNotLocked) {
        if (sender.compare_exchange_weak(expected, kLockedNoWaiters, std::memory_order_acquire,
                                         std::memory_order_relaxed)) {
          return false;
        }
      } else {
        new_state.next = reinterpret_cast<BaseCore*>(expected);
        if (sender.compare_exchange_weak(expected, reinterpret_cast<std::uint64_t>(&new_state),
                                         std::memory_order_release, std::memory_order_relaxed)) {
          return true;  // need suspend
        }
      }
    }
  }

  template <bool FIFO>
  [[nodiscard]] BaseCore* TryUnlock() noexcept {
    YACLIB_DEBUG(sender.load(std::memory_order_relaxed) == kNotLocked, "UnlockHere must be called after Lock!");
    if (receiver != nullptr) {
      return receiver;
    }
    const auto old_batch_here = std::exchange(batch_here, 0);
    auto expected = kLockedNoWaiters;
    if (sender.load(std::memory_order_relaxed) == expected &&
        sender.compare_exchange_strong(expected, kNotLocked, std::memory_order_release, std::memory_order_relaxed)) {
      return nullptr;
    }

    expected = sender.exchange(kLockedNoWaiters, std::memory_order_acquire);
    batch_here = old_batch_here + 1;
    if constexpr (FIFO) {
      Node* node = reinterpret_cast<BaseCore*>(expected);
      Node* prev = nullptr;
      do {
        auto* next = node->next;
        node->next = prev;
        prev = node;
        node = next;
      } while (node != nullptr);
      return static_cast<BaseCore*>(prev);
    } else {
      return reinterpret_cast<BaseCore*>(expected);
    }
  }

  template <bool FIFO>
  void UnlockHereImpl() noexcept {
    auto* next = TryUnlock<FIFO>();
    if (next == nullptr) {
      return;
    }
    batch_here = 0;
    receiver = static_cast<detail::BaseCore*>(next->next);
    // next->_executor for next critical section
    next->_executor->Submit(*next);
  }

  template <bool FIFO, std::uint8_t BatchHere>
  auto UnlockOnImpl(BaseCore& curr, IExecutor& executor) noexcept {
    // _executor for current coroutine resume
    auto curr_executor = std::exchange(curr._executor, &executor);
    executor.Submit(curr);
    auto* next = TryUnlock<FIFO>();
    if (next == nullptr) {
      YACLIB_SUSPEND();
    }
    receiver = static_cast<BaseCore*>(next->next);
    if (batch_here > BatchHere) {
      batch_here = 0;
      // next->_executor for next critical section
      next->_executor->Submit(*next);
      YACLIB_SUSPEND();
    }
    // curr_executor for next critical section
    next->_executor = std::move(curr_executor);
    YACLIB_TRANSFER(next->Curr());
  }

  static constexpr std::uint64_t kNotLocked = 1;
  static constexpr std::uint64_t kLockedNoWaiters = 0;

  // locked without waiters, not locked, otherwise - head of the waiters list
  yaclib_std::atomic_uint64_t sender = kNotLocked;
  BaseCore* receiver = nullptr;
  std::uint8_t batch_here = 0;
};

class [[nodiscard]] LockAwaiter {
 public:
  explicit LockAwaiter(MutexImpl& m) noexcept : _mutex{m} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    return _mutex.TryLock();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _mutex.Lock(handle.promise());
  }

  constexpr void await_resume() noexcept {
  }

 protected:
  MutexImpl& _mutex;
};

template <bool FIFO, std::uint8_t BatchHere>
class [[nodiscard]] UnlockAwaiter final {
 public:
  explicit UnlockAwaiter(MutexImpl& m) noexcept : _mutex{m} {
  }

  bool await_ready() noexcept {
    auto* next = _mutex.TryUnlock<FIFO>();
    if (next == nullptr) {
      return true;
    }
    _mutex.receiver = next;
    return false;
  }

  template <typename Promise>
  auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& curr = handle.promise();
    auto& next = *_mutex.receiver;
    _mutex.receiver = static_cast<BaseCore*>(next.next);
    if (_mutex.batch_here > BatchHere) {
      _mutex.batch_here = 0;
      // next._executor for next critical section
      // curr._executor for current coroutine resume
      next._executor->Submit(next);
      YACLIB_RESUME(handle);
    }
    // curr._executor for next critical section
    // next._executor for current coroutine resume
    curr._executor.Swap(next._executor);
    curr._executor->Submit(curr);
    YACLIB_TRANSFER(next.Curr());
  }

  constexpr void await_resume() noexcept {
  }

 private:
  MutexImpl& _mutex;
};

template <bool FIFO, std::uint8_t BatchHere>
class [[nodiscard]] UnlockOnAwaiter final {
 public:
  explicit UnlockOnAwaiter(MutexImpl& m, IExecutor& e) noexcept : _mutex{m}, _executor{e} {
  }

  constexpr bool await_ready() noexcept {
    return false;
  }

  template <typename Promise>
  auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _mutex.UnlockOnImpl<FIFO, BatchHere>(handle.promise(), _executor);
  }

  constexpr void await_resume() noexcept {
  }

 private:
  MutexImpl& _mutex;
  IExecutor& _executor;
};

}  // namespace detail

/**
 * Mutex for coroutines
 *
 * \note It does not block execution thread, only coroutine
 */
template <bool DefaultFIFO = false, std::uint8_t DefaultBatchHere = 1>
class Mutex final : protected detail::MutexImpl {
 public:
  Mutex(Mutex&&) = delete;
  Mutex(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  Mutex() noexcept = default;
  ~Mutex() noexcept = default;

  /**
   * Try to lock mutex and create LockGuard for it
   *
   * \note If we couldn't lock mutex, then LockGuard::OwnsLock() will be false
   * \return Awaitable, which await_resume returns the LockGuard
   */
  auto TryGuard() noexcept {
    return GuardUnique{*this, std::try_to_lock_t{}};
  }

  /**
   * Lock mutex and create LockGuard for it
   *
   * \return Awaitable, which await_resume returns the LockGuard
   */
  auto UniqueGuard() noexcept {
    return GuardUniqueAwaiter{*this};
  }

  /**
   * Lock mutex and create LockGuard for it
   *
   * \return Awaitable, which await_resume returns the LockGuard
   */
  auto StickyGuard() noexcept {
    return GuardStickyAwaiter{*this};
  }

  /**
   * Try to lock mutex
   * return true if mutex was locked, false otherwise
   */
  using detail::MutexImpl::TryLock;

  /**
   * Lock mutex
   */
  auto Lock() noexcept {
    return detail::LockAwaiter{*this};
  }

  /**
   * The best way to unlock mutex, if you interested in batched critical section
   */
  template <bool FIFO = DefaultFIFO, std::uint8_t BatchHere = DefaultBatchHere>
  auto Unlock() noexcept {
    return detail::UnlockAwaiter<FIFO, BatchHere>{*this};
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
  template <bool FIFO = DefaultFIFO, std::uint8_t BatchHere = DefaultBatchHere>
  auto UnlockOn(IExecutor& e) noexcept {
    return detail::UnlockOnAwaiter<FIFO, BatchHere>{*this, e};
  }

  /**
   * The general way to unlock mutex, mainly for RAII
   */
  template <bool FIFO = DefaultFIFO>
  void UnlockHere() noexcept {
    return UnlockHereImpl<FIFO>();
  }

  class GuardUnique;
  class GuardUniqueAwaiter;

  class GuardSticky;
  class GuardStickyAwaiter;
  class LockStickyAwaiter;
};

}  // namespace yaclib

#pragma once

#include <yaclib/coro/guard_unique.hpp>
#include <yaclib/coro/mutex.hpp>

namespace yaclib {
namespace detail {

template <bool FIFO, std::uint8_t BatchHere>
class [[nodiscard]] UnlockStickyAwaiter {
 public:
  UnlockStickyAwaiter(MutexImpl& m, IExecutor* e) noexcept : _mutex{m}, _executor{e} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    if (_executor) {
      return false;
    }
    _mutex.UnlockHereImpl<FIFO>();
    return true;
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    YACLIB_ASSERT(_executor != nullptr);
    return _mutex.UnlockOnImpl<FIFO, BatchHere>(handle.promise(), *_executor);
  }

  constexpr void await_resume() noexcept {
  }

 private:
  MutexImpl& _mutex;
  IExecutor* _executor;
};

}  // namespace detail

template <bool DefaultFIFO, std::uint8_t DefaultBatchHere>
class [[nodiscard]] Mutex<DefaultFIFO, DefaultBatchHere>::LockStickyAwaiter {
 public:
  explicit LockStickyAwaiter(GuardSticky& g) noexcept : _guard{g} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    _guard._executor = nullptr;
    return _guard._mutex.TryLock();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& promise = handle.promise();
    _guard._executor = promise._executor.Get();
    if (_guard._mutex.Lock(promise)) {
      return true;  // suspend
    }
    _guard._executor = nullptr;
    return false;
  }

  constexpr void await_resume() noexcept {
  }

 protected:
  GuardSticky& _guard;
};

template <bool DefaultFIFO, std::uint8_t DefaultBatchHere>
class [[nodiscard]] Mutex<DefaultFIFO, DefaultBatchHere>::GuardSticky : public GuardUnique {
 public:
  GuardSticky(GuardSticky&& other) noexcept : GuardUnique{other}, _executor{other._executor} {
  }

  GuardSticky& operator=(GuardSticky&& other) noexcept {
    Swap(other);
    return *this;
  }

  auto Lock() noexcept {
    YACLIB_ERROR(this->_owns, "Cannot lock already locked mutex");
    this->_owns = true;
    return LockStickyAwaiter{*this};
  }

  template <bool FIFO = DefaultFIFO, std::uint8_t BatchHere = DefaultBatchHere>
  auto Unlock() noexcept {
    return detail::UnlockStickyAwaiter<FIFO, BatchHere>{this->_mutex, _executor};
  }

  void Swap(GuardSticky& other) noexcept {
    GuardUnique::Swap(other);
    std::swap(_executor, other._executor);
  }

 protected:
  IExecutor* _executor = nullptr;
};

template <bool DefaultFIFO, std::uint8_t DefaultBatchHere>
class [[nodiscard]] Mutex<DefaultFIFO, DefaultBatchHere>::GuardStickyAwaiter {
 public:
  explicit GuardStickyAwaiter(Mutex& m) : _guard{m, std::adopt_lock} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    LockStickyAwaiter awaiter{_guard};
    return awaiter.await_ready();
  }

  YACLIB_INLINE bool await_suspend() noexcept {
    LockStickyAwaiter awaiter{_guard};
    return awaiter.await_suspend();
  }

  YACLIB_INLINE GuardSticky await_resume() noexcept {
    return std::move(_guard);
  }

 private:
  GuardSticky _guard;
};

}  // namespace yaclib

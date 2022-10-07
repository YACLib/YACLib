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
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
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
    _guard.executor = nullptr;
    return _guard._mutex->TryLock();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& promise = handle.promise();
    _guard.executor = promise._executor.Get();
    if (static_cast<detail::MutexImpl*>(_guard._mutex)->Lock(promise)) {
      return true;  // suspend
    }
    _guard.executor = nullptr;
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
  using GuardUnique::GuardUnique;

  GuardSticky(GuardSticky&& other) noexcept : GuardUnique{std::move(other)}, executor{other.executor} {
  }

  GuardSticky& operator=(GuardSticky&& other) noexcept {
    Swap(other);
    return *this;
  }

  auto Lock() noexcept {
    YACLIB_ERROR(this->_owns, "Cannot LockSticky already locked mutex");
    this->_owns = true;
    return LockStickyAwaiter{*this};
  }

  template <bool FIFO = DefaultFIFO, std::uint8_t BatchHere = DefaultBatchHere>
  auto Unlock() noexcept {
    YACLIB_ERROR(!this->_owns, "Cannot UnlockSticky not locked mutex");
    this->_owns = false;
    return detail::UnlockStickyAwaiter<FIFO, BatchHere>{*this->_mutex, executor};
  }

  void Swap(GuardSticky& other) noexcept {
    GuardUnique::Swap(other);
    std::swap(executor, other._executor);
  }

  IExecutor* executor = nullptr;
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

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    LockStickyAwaiter awaiter{_guard};
    return awaiter.await_suspend(handle);
  }

  YACLIB_INLINE GuardSticky await_resume() noexcept {
    YACLIB_ASSERT(_guard.OwnsLock());
    return std::move(_guard);
  }

 private:
  GuardSticky _guard;
};

}  // namespace yaclib

#pragma once

#include <yaclib/config.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {
namespace detail {

template <typename Mutex>
class [[nodiscard]] LockStickyAwaiter {
 public:
  explicit LockStickyAwaiter(Mutex& m, IExecutor*& e) noexcept : _mutex{m}, _executor{e} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    _executor = nullptr;
    return _mutex.TryLockAwait();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& promise = handle.promise();
    _executor = promise._executor.Get();
    if (_mutex.AwaitLock(promise)) {
      return true;
    }
    _executor = nullptr;
    return false;
  }

  constexpr void await_resume() noexcept {
  }

 protected:
  Mutex& _mutex;
  IExecutor*& _executor;
};

template <typename Mutex>
class [[nodiscard]] UnlockStickyAwaiter {
 public:
  explicit UnlockStickyAwaiter(Mutex& m, IExecutor* e) noexcept : _mutex{m}, _executor{e} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    if (_executor != nullptr) {
      return false;
    }
    _mutex.UnlockHere();
    return true;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    YACLIB_ASSERT(_executor != nullptr);
    return _mutex.AwaitUnlockOn(handle.promise(), *_executor);
  }

  constexpr void await_resume() noexcept {
  }

 private:
  Mutex& _mutex;
  IExecutor* _executor;
};

template <typename>
class GuardStickyAwaiter;
;
}  // namespace detail

template <typename Mutex>
class [[nodiscard]] StickyGuard : public detail::Guard<Mutex, false> {
  using Base = detail::Guard<Mutex, false>;

 public:
  using MutexType = Mutex;
  using Base::Base;

  StickyGuard(StickyGuard&& other) noexcept : Base{std::move(other)}, _executor{other._executor} {
  }

  StickyGuard& operator=(StickyGuard&& other) noexcept {
    Swap(other);
    return *this;
  }

  auto Lock() noexcept {
    auto* m = static_cast<Mutex*>(Base::LockState());
    auto& mutex_impl = Mutex::template Cast<typename Mutex::Base>(*m);
    return detail::LockStickyAwaiter{mutex_impl, _executor};
  }

  auto Unlock() noexcept {
    auto* m = static_cast<Mutex*>(Base::UnlockState());
    auto& mutex_impl = Mutex::template Cast<typename Mutex::Base>(*m);
    return detail::UnlockStickyAwaiter{mutex_impl, _executor};
  }

  void Swap(StickyGuard& other) noexcept {
    Base::Swap(other);
    std::swap(_executor, other._executor);
  }

 private:
  IExecutor* _executor = nullptr;

  template <typename>
  friend class detail::GuardStickyAwaiter;
};

namespace detail {

template <typename Mutex>
class [[nodiscard]] GuardStickyAwaiter {
 public:
  explicit GuardStickyAwaiter(Mutex& m) : _guard{m, std::adopt_lock} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    auto& mutex_impl = Mutex::template Cast<typename Mutex::Base>(*_guard.Mutex());
    LockStickyAwaiter awaiter{mutex_impl, _guard._executor};
    return awaiter.await_ready();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& mutex_impl = Mutex::template Cast<typename Mutex::Base>(*_guard.Mutex());
    LockStickyAwaiter awaiter{mutex_impl, _guard._executor};
    return awaiter.await_suspend(handle);
  }

  YACLIB_INLINE auto await_resume() noexcept {
    YACLIB_ASSERT(_guard);
    return std::move(_guard);
  }

 private:
  StickyGuard<Mutex> _guard;
};

}  // namespace detail
}  // namespace yaclib

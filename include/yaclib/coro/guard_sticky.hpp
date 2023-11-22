#pragma once

#include <yaclib/config.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {
namespace detail {

template <typename M>
class [[nodiscard]] LockStickyAwaiter {
 public:
  explicit LockStickyAwaiter(M& m, IExecutor*& e) noexcept : _mutex{m}, _executor{e} {
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
  M& _mutex;
  IExecutor*& _executor;
};

template <typename M>
class [[nodiscard]] UnlockStickyAwaiter {
 public:
  explicit UnlockStickyAwaiter(M& m, IExecutor* e) noexcept : _mutex{m}, _executor{e} {
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
  M& _mutex;
  IExecutor* _executor;
};

template <typename>
class GuardStickyAwaiter;
;
}  // namespace detail

template <typename M>
class [[nodiscard]] StickyGuard : public detail::Guard<M, false> {
  using Base = detail::Guard<M, false>;

 public:
  using MutexType = M;
  using Base::Base;

  StickyGuard(StickyGuard&& other) noexcept : Base{std::move(other)}, _executor{other._executor} {
  }

  StickyGuard& operator=(StickyGuard&& other) noexcept {
    Swap(other);
    return *this;
  }

  auto Lock() noexcept {
    auto* m = static_cast<M*>(Base::LockState());
    auto& base = M::template Cast<typename M::Base>(*m);
    return detail::LockStickyAwaiter{base, _executor};
  }

  auto Unlock() noexcept {
    auto* m = static_cast<M*>(Base::UnlockState());
    auto& base = M::template Cast<typename M::Base>(*m);
    return detail::UnlockStickyAwaiter{base, _executor};
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

template <typename M>
class [[nodiscard]] GuardStickyAwaiter {
 public:
  explicit GuardStickyAwaiter(M& m) : _guard{m, std::adopt_lock} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    auto& mutex_impl = M::template Cast<typename M::Base>(*_guard.Mutex());
    LockStickyAwaiter awaiter{mutex_impl, _guard._executor};
    return awaiter.await_ready();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& mutex_impl = M::template Cast<typename M::Base>(*_guard.Mutex());
    LockStickyAwaiter awaiter{mutex_impl, _guard._executor};
    return awaiter.await_suspend(handle);
  }

  YACLIB_INLINE auto await_resume() noexcept {
    YACLIB_ASSERT(_guard);
    return std::move(_guard);
  }

 private:
  StickyGuard<M> _guard;
};

}  // namespace detail
}  // namespace yaclib

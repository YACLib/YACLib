#pragma once

#include <yaclib/config.hpp>
#include <yaclib/coro/coro.hpp>

#include <mutex>

namespace yaclib::detail {

template <typename M, bool Shared = false>
class [[nodiscard]] LockAwaiter {
 public:
  explicit LockAwaiter(M& m) noexcept : _mutex{m} {
  }

  YACLIB_INLINE bool await_ready() noexcept {
    if constexpr (Shared) {
      return _mutex.TryLockSharedAwait();
    } else {
      return _mutex.TryLockAwait();
    }
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    if constexpr (Shared) {
      return _mutex.AwaitLockShared(handle.promise());
    } else {
      return _mutex.AwaitLock(handle.promise());
    }
  }

  constexpr void await_resume() noexcept {
  }

 protected:
  M& _mutex;
};

template <template <typename> typename Guard, typename M, bool Shared = false>
class [[nodiscard]] GuardAwaiter : public LockAwaiter<typename M::Base, Shared> {
 public:
  using LockAwaiter<typename M::Base, Shared>::LockAwaiter;

  YACLIB_INLINE auto await_resume() noexcept {
    return Guard<M>{M::template Cast<M>(this->_mutex), std::adopt_lock};
  }
};

}  // namespace yaclib::detail

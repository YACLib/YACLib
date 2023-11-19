#pragma once

#include <yaclib/config.hpp>
#include <yaclib/coro/coro.hpp>

#include <mutex>

namespace yaclib::detail {

template <typename Mutex, bool Shared = false>
class [[nodiscard]] LockAwaiter {
 public:
  explicit LockAwaiter(Mutex& m) noexcept : _mutex{m} {
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
  Mutex& _mutex;
};

template <template <typename> typename Guard, typename Mutex, bool Shared = false>
class [[nodiscard]] GuardAwaiter : public LockAwaiter<typename Mutex::Base, Shared> {
 public:
  using LockAwaiter<typename Mutex::Base, Shared>::LockAwaiter;

  YACLIB_INLINE auto await_resume() noexcept {
    return Guard<Mutex>{Mutex::template Cast<Mutex>(this->_mutex), std::adopt_lock};
  }
};

}  // namespace yaclib::detail

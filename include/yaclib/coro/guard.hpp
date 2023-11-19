#pragma once

#include <yaclib/coro/detail/guard_state.hpp>
#include <yaclib/exe/executor.hpp>

#include <mutex>

namespace yaclib {
namespace detail {

template <typename M, bool Shared>
class Guard : protected detail::GuardState {
 public:
  Guard() noexcept = default;
  Guard(Guard&& other) noexcept : GuardState{std::move(other)} {
  }
  explicit Guard(M& m, std::defer_lock_t) noexcept : GuardState{&m, false} {
  }
  explicit Guard(M& m, std::try_to_lock_t) noexcept : GuardState{&m, m.TryLock()} {
  }
  explicit Guard(M& m, std::adopt_lock_t) noexcept : GuardState{&m, true} {
  }

  ~Guard() noexcept {
    if (*this) {
      UnlockHere();
    }
  }

  Guard& operator=(Guard&& other) noexcept {
    Swap(other);
    return *this;
  }

  auto Lock() noexcept {
    M* m = LockState();
    if constexpr (Shared) {
      return m->LockShared();
    } else {
      return m->Lock();
    }
  }

  bool TryLock() noexcept {
    M* m = LockState();
    if constexpr (Shared) {
      if (m->TryLockShared()) {
        return true;
      }
    } else {
      if (m->TryLock()) {
        return true;
      }
    }
    UnlockState();
    return false;
  }

  auto Unlock() noexcept {
    M* m = UnlockState();
    if constexpr (Shared) {
      return m->UnlockShared();
    } else {
      return m->Unlock();
    }
  }

  auto UnlockOn(IExecutor& e) noexcept {
    M* m = UnlockState();
    if constexpr (Shared) {
      return m->UnlockOnShared(e);
    } else {
      return m->UnlockOn(e);
    }
  }

  void UnlockHere() noexcept {
    M* m = UnlockState();
    if constexpr (Shared) {
      m->UnlockHereShared();
    } else {
      m->UnlockHere();
    }
  }

  void Swap(Guard& other) noexcept {
    std::swap(_state, other._state);
  }

  M* Release() noexcept {
    return ReleaseState();
  }

  M* Mutex() const noexcept {
    return Ptr();
  }

  [[nodiscard]] bool OwnsLock() const noexcept {
    return Owns();
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return OwnsLock();
  }
};

}  // namespace detail

template <typename Mutex>
class [[nodiscard]] UniqueGuard : public detail::Guard<Mutex, false> {
 public:
  using MutexType = Mutex;
  using detail::Guard<Mutex, false>::Guard;
};

template <typename Mutex>
class [[nodiscard]] SharedGuard : public detail::Guard<Mutex, true> {
 public:
  using MutexType = Mutex;
  using detail::Guard<Mutex, true>::Guard;
};

}  // namespace yaclib

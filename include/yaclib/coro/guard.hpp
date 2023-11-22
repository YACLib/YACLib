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
  explicit Guard(M& m, std::try_to_lock_t) noexcept : GuardState{&m, TryLockImpl(m)} {
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
    auto* m = static_cast<M*>(LockState());
    if constexpr (Shared) {
      return m->LockShared();
    } else {
      return m->Lock();
    }
  }

  bool TryLock() noexcept {
    auto* m = static_cast<M*>(LockState());
    if (TryLockImpl(*m)) {
      return true;
    }
    UnlockState();
    return false;
  }

  auto Unlock() noexcept {
    auto* m = static_cast<M*>(UnlockState());
    if constexpr (Shared) {
      return m->UnlockShared();
    } else {
      return m->Unlock();
    }
  }

  auto UnlockOn(IExecutor& e) noexcept {
    auto* m = static_cast<M*>(UnlockState());
    if constexpr (Shared) {
      return m->UnlockOnShared(e);
    } else {
      return m->UnlockOn(e);
    }
  }

  void UnlockHere() noexcept {
    auto* m = static_cast<M*>(UnlockState());
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
    return static_cast<M*>(ReleaseState());
  }

  M* Mutex() const noexcept {
    return static_cast<M*>(Ptr());
  }

  [[nodiscard]] bool OwnsLock() const noexcept {
    return Owns();
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return OwnsLock();
  }

 private:
  static bool TryLockImpl(M& m) {
    if constexpr (Shared) {
      return m.TryLockShared();
    } else {
      return m.TryLock();
    }
  }
};

}  // namespace detail

template <typename M>
class [[nodiscard]] UniqueGuard : public detail::Guard<M, false> {
 public:
  using MutexType = M;
  using detail::Guard<M, false>::Guard;
};

template <typename M>
class [[nodiscard]] SharedGuard : public detail::Guard<M, true> {
 public:
  using MutexType = M;
  using detail::Guard<M, true>::Guard;
};

}  // namespace yaclib

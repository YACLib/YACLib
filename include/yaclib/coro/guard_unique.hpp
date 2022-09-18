#pragma once

#include <yaclib/coro/mutex.hpp>

namespace yaclib {

template <bool DefaultFIFO, std::uint8_t DefaultBatchHere>
class [[nodiscard]] Mutex<DefaultFIFO, DefaultBatchHere>::GuardUnique {
 public:
  explicit GuardUnique(Mutex& m, std::defer_lock_t) noexcept : _mutex{&m}, _owns{false} {
  }
  explicit GuardUnique(Mutex& m, std::try_to_lock_t) noexcept : _mutex{&m}, _owns{m.TryLock()} {
  }
  explicit GuardUnique(Mutex& m, std::adopt_lock_t) noexcept : _mutex{&m}, _owns{true} {
  }

  GuardUnique(GuardUnique&& other) noexcept : _mutex{other._mutex}, _owns{other._owns} {
    other._owns = false;
  }

  GuardUnique& operator=(GuardUnique&& other) noexcept {
    Swap(other);
    return *this;
  }

  [[nodiscard]] bool OwnsLock() const noexcept {
    return _owns;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return OwnsLock();
  }

  auto Lock() noexcept {
    YACLIB_ERROR(_owns, "Cannot lock already locked mutex");
    _owns = true;
    return _mutex->Lock();
  }

  template <bool FIFO = DefaultFIFO, std::uint8_t BatchHere = DefaultBatchHere>
  auto Unlock() noexcept {
    YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
    _owns = false;
    return _mutex->Unlock<FIFO, BatchHere>();
  }

  template <bool FIFO = DefaultFIFO, std::uint8_t BatchHere = DefaultBatchHere>
  auto UnlockOn(IExecutor& e) noexcept {
    YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
    _owns = false;
    return _mutex->UnlockOn<FIFO, BatchHere>(e);
  }

  template <bool FIFO = DefaultFIFO>
  void UnlockHere() noexcept {
    YACLIB_ERROR(!_owns, "Cannot unlock not locked mutex");
    _owns = false;
    _mutex->UnlockHere<FIFO>();
  }

  void Swap(GuardUnique& other) noexcept {
    std::swap(_mutex, other._mutex);
    std::swap(_owns, other._owns);
  }

  Mutex* Release() noexcept {
    _owns = false;
    return _mutex;
  }

  ~GuardUnique() noexcept {
    if (_owns) {
      YACLIB_WARN(std::uncaught_exceptions() == 0, "Better to use co_await unique_guard.Unlock()");
      _mutex->UnlockHere();
    }
  }

 protected:
  Mutex* _mutex;
  bool _owns;  // TODO(MBkkt) add as _mutex bit
};

template <bool DefaultFIFO, std::uint8_t DefaultBatchHere>
class [[nodiscard]] Mutex<DefaultFIFO, DefaultBatchHere>::GuardUniqueAwaiter : public detail::LockAwaiter {
 public:
  using LockAwaiter::LockAwaiter;

  YACLIB_INLINE GuardUnique await_resume() noexcept {
    return GuardUnique{static_cast<Mutex<DefaultFIFO, DefaultBatchHere>&>(_mutex), std::adopt_lock};
  }
};

}  // namespace yaclib

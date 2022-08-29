#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/detail/fiber/queue.hpp>
#include <yaclib/fault/inject.hpp>

namespace yaclib::detail::fiber {

template <typename T>
class AtomicWait {
 public:
  AtomicWait() noexcept = default;

  constexpr AtomicWait(T desired) noexcept : _value(desired) {
  }

  T operator=(T desired) volatile noexcept {
    return _value = desired;
  }

  T operator=(T desired) noexcept {
    return _value = desired;
  }

  [[nodiscard]] bool is_lock_free() const volatile noexcept {
    return true;
  }

  [[nodiscard]] bool is_lock_free() const noexcept {
    return true;
  }

  static constexpr bool is_always_lock_free = true;

#if YACLIB_FUTEX != 0
  void wait(T old, std::memory_order) const noexcept {
    while (_value == old) {
      const_cast<FiberQueue*>(&_queue)->Wait(NoTimeoutTag{});
    }
  }
  void wait(T old, std::memory_order) const volatile noexcept {
    while (_value == old) {
      const_cast<FiberQueue*>(&_queue)->Wait(NoTimeoutTag{});
    }
  }

  void notify_one() noexcept {
    _queue.NotifyOne();
  }
  void notify_one() volatile noexcept {
    const_cast<FiberQueue*>(&_queue)->NotifyOne();
  }

  void notify_all() noexcept {
    _queue.NotifyAll();
  }
  void notify_all() volatile noexcept {
    const_cast<FiberQueue*>(&_queue)->NotifyAll();
  }
#endif

 protected:
  T _value;
#if YACLIB_FUTEX != 0
  FiberQueue _queue;
#endif
};

}  // namespace yaclib::detail::fiber

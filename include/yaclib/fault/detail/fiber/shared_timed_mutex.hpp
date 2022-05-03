#pragma once

#include <yaclib/fault/detail/fiber/shared_mutex.hpp>

#include <yaclib_std/chrono>

namespace yaclib::detail::fiber {

class SharedTimedMutex : public SharedMutex {
  using Base = SharedMutex;

 public:
  using Base::Base;

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    bool r = true;
    if (_occupied) {
      r = !_exclusive_queue.Wait(timeout_duration);
    }
    if (r) {
      LockHelper();
    }
    return r;
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::duration<Clock, Duration>& timeout_time) {
    bool r = true;
    if (_occupied) {
      r = !_exclusive_queue.Wait(timeout_time);
    }
    if (r) {
      LockHelper();
    }
    return r;
  }

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    bool r = true;
    if (_occupied && _exclusive_mode) {
      r = !_shared_queue.Wait(timeout_duration);
    }
    if (r) {
      SharedLockHelper();
    }
    return r;
  }

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::duration<Clock, Duration>& timeout_time) {
    bool r = true;
    if (_occupied && _exclusive_mode) {
      r = !_shared_queue.Wait(timeout_time);
    }
    if (r) {
      SharedLockHelper();
    }
    return r;
  }
};

}  // namespace yaclib::detail::fiber

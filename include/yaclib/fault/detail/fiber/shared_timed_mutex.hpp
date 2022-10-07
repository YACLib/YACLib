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
    return TimedWaitHelper(timeout_duration, true);
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return TimedWaitHelper(timeout_time, true);
  }

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    return TimedWaitHelper(timeout_duration, false);
  }

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return TimedWaitHelper(timeout_time, false);
  }

 private:
  template <typename Timeout>
  bool TimedWaitHelper(const Timeout& timeout, bool exclusive) {
    bool r = true;
    if (_occupied && (exclusive || _exclusive_mode)) {
      if (exclusive) {
        r = _exclusive_queue.Wait(timeout) == WaitStatus::Ready;
      } else {
        r = _shared_queue.Wait(timeout) == WaitStatus::Ready;
      }
    }
    YACLIB_DEBUG(r && _occupied && (exclusive || _exclusive_mode), "about to be locked twice and not in a good way");
    if (r) {
      SharedLockHelper();
    }
    return r;
  }
};

}  // namespace yaclib::detail::fiber

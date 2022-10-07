#pragma once

#include <yaclib/fault/detail/fiber/mutex.hpp>
#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail::fiber {

class TimedMutex : public Mutex {
  using Base = Mutex;

 public:
  using Base::Base;

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    return TimedWaitHelper(timeout_duration);
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return TimedWaitHelper(timeout_time);
  }

 private:
  template <typename Timeout>
  bool TimedWaitHelper(const Timeout& timeout) {
    bool r = true;
    if (_occupied) {
      r = _queue.Wait(timeout) == WaitStatus::Ready;
    }
    YACLIB_DEBUG(r && _occupied, "about to be locked twice");
    if (r) {
      _occupied = true;
    }
    return r;
  }
};

}  // namespace yaclib::detail::fiber

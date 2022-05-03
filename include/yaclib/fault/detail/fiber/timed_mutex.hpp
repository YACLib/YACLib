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
    bool r = true;
    if (_occupied) {
      r = !_queue.Wait(timeout_duration);
    }
    if (r) {
      _occupied = true;
    }
    return r;
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::duration<Clock, Duration>& timeout_time) {
    bool r = true;
    if (_occupied) {
      r = !_queue.Wait(timeout_time);
    }
    if (r) {
      _occupied = true;
    }
    return r;
  }
};

}  // namespace yaclib::detail::fiber

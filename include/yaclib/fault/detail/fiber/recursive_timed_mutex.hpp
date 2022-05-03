#pragma once

#include <yaclib/fault/detail/fiber/queue.hpp>
#include <yaclib/fault/detail/fiber/recursive_mutex.hpp>

namespace yaclib::detail::fiber {

class RecursiveTimedMutex : public RecursiveMutex {
  using Base = RecursiveMutex;

 public:
  using Base::Base;

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    bool r = true;
    if (_occupied_count != 0 && _owner_id != fault::Scheduler::GetId()) {
      r = !_queue.Wait(timeout_duration);
    }
    if (r) {
      LockHelper();
    }
    return r;
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::duration<Clock, Duration>& timeout_time) {
    bool r = true;
    if (_occupied_count != 0 && _owner_id != fault::Scheduler::GetId()) {
      r = !_queue.Wait(timeout_time);
    }
    if (r) {
      LockHelper();
    }
    return r;
  }
};

}  // namespace yaclib::detail::fiber

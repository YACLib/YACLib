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
    if (_occupied_count != 0 && _owner_id != fault::Scheduler::GetId()) {
      r = _queue.Wait(timeout) == WaitStatus::Ready;
    }
    YACLIB_DEBUG(r && (_occupied_count != 0 && _owner_id != fault::Scheduler::GetId()),
                 "about to be locked twice and not in a good way");
    if (r) {
      LockHelper();
    }
    return r;
  }
};

}  // namespace yaclib::detail::fiber

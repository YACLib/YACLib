#pragma once

#include <yaclib/fault/inject.hpp>

#include <yaclib_std/chrono>
#include <yaclib_std/mutex>

namespace yaclib::detail {

// TODO(myannyax) Maybe implement wait overloads with stop_token

template <typename Impl>
class ConditionVariableAny : private Impl {
 public:
  using Impl::Impl;

  void notify_one() noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_one());
  }
  void notify_all() noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_all());
  }

  template <typename Lock>
  void wait(Lock& lock) {
    YACLIB_INJECT_FAULT(Impl::wait(lock));
  }
  template <typename Lock, typename Predicate>
  void wait(Lock& lock, Predicate&& stop_waiting) {
    YACLIB_INJECT_FAULT(Impl::wait(lock, std::forward<Predicate>(stop_waiting)));
  }

  template <typename Lock, typename Rep, typename Period>
  std::cv_status wait_for(Lock& lock, const std::chrono::duration<Rep, Period>& rel_time) {
    YACLIB_INJECT_FAULT(auto r = Impl::wait_for(lock, rel_time));
    return r;
  }
  template <typename Lock, typename Rep, typename Period, typename Predicate>
  bool wait_for(Lock& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate&& stop_waiting) {
    YACLIB_INJECT_FAULT(auto r = Impl::wait_for(lock, rel_time, std::forward<Predicate>(stop_waiting)));
    return r;
  }

  template <typename Lock, typename Clock, typename Duration>
  std::cv_status wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& timeout_time) {
    YACLIB_INJECT_FAULT(auto r = Impl::wait_until(lock, timeout_time));
    return r;
  }
  template <typename Lock, typename Clock, typename Duration, typename Predicate>
  bool wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& timeout_time, Predicate&& stop_waiting) {
    YACLIB_INJECT_FAULT(auto r = Impl::wait_until(lock, timeout_time, std::forward<Predicate>(stop_waiting)));
    return r;
  }
};

}  // namespace yaclib::detail

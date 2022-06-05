#pragma once

#include <yaclib/fault/detail/shared_mutex.hpp>
#include <yaclib/fault/inject.hpp>

#include <yaclib_std/chrono>

namespace yaclib::detail {

// TODO(myannyax) Make mixins for mutex like types, here we want:
//  private Impl, public SharedMutex<Impl>, public TimedMutex<Impl>
template <typename Impl>
class SharedTimedMutex : public SharedMutex<Impl> {
  using Base = SharedMutex<Impl>;

 public:
  using Base::Base;

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    YACLIB_INJECT_FAULT(auto r = Impl::try_lock_for(timeout_duration));
    return r;
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    YACLIB_INJECT_FAULT(auto r = Impl::try_lock_until(timeout_time));
    return r;
  }

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    YACLIB_INJECT_FAULT(auto r = Impl::try_lock_shared_for(timeout_duration));
    return r;
  }

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    YACLIB_INJECT_FAULT(auto r = Impl::try_lock_shared_until(timeout_time));
    return r;
  }
};

}  // namespace yaclib::detail

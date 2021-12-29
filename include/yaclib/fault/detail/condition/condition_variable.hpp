#pragma once

#include <yaclib/fault/mutex.hpp>

#include <condition_variable>

namespace yaclib::detail {

class ConditionVariable {
 public:
  /*constexpr*/ ConditionVariable() /*noexcept*/ = default;

  ~ConditionVariable() = default;

  ConditionVariable(const ConditionVariable&) = delete;
  ConditionVariable& operator=(const ConditionVariable&) = delete;

  void notify_one() noexcept;
  void notify_all() noexcept;

  void wait(std::unique_lock<yaclib_std::mutex>& lock) noexcept;

  template <typename Predicate>
  void wait(std::unique_lock<yaclib_std::mutex>& lock, Predicate predicate) {
    YACLIB_INJECT_FAULT(_impl.wait(lock, predicate));
  }

  template <typename Clock, typename Duration>
  std::cv_status wait_until(std::unique_lock<yaclib_std::mutex>& lock,
                            const std::chrono::time_point<Clock, Duration>& time_point) {
    YACLIB_INJECT_FAULT(auto result = _impl.wait_until(lock, time_point));
    return result;
  }

  template <typename Clock, typename Duration, typename Predicate>
  bool wait_until(std::unique_lock<yaclib_std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& time_point,
                  Predicate predicate) {
    YACLIB_INJECT_FAULT(auto result = _impl.wait_until(lock, time_point, predicate));
    return result;
  }

  template <typename Rep, typename Period>
  std::cv_status wait_for(std::unique_lock<yaclib_std::mutex>& lock,
                          const std::chrono::duration<Rep, Period>& duration) {
    YACLIB_INJECT_FAULT(auto result = _impl.wait_for(lock, duration));
    return result;
  }

  template <typename Rep, typename Period, typename Predicate>
  bool wait_for(std::unique_lock<yaclib_std::mutex>& lock, const std::chrono::duration<Rep, Period>& duration,
                Predicate predicate) {
    YACLIB_INJECT_FAULT(auto result = _impl.wait_for(lock, duration, predicate));
    return result;
  }

  /*using native_handle_type = std::condition_variable::native_handle_type;

  native_handle_type native_handle();*/

 private:
  std::condition_variable_any _impl;
};
}  // namespace yaclib::detail

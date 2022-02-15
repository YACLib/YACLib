#pragma once

#include <yaclib/fault/mutex.hpp>

#include <condition_variable>

namespace yaclib::detail {

class ConditionVariable {
 public:
  ConditionVariable() noexcept = default;

  ~ConditionVariable() = default;

  ConditionVariable(const ConditionVariable&) = delete;
  ConditionVariable& operator=(const ConditionVariable&) = delete;

  void notify_one() noexcept;
  void notify_all() noexcept;

  void wait(std::unique_lock<yaclib::detail::Mutex>& lock) noexcept;

  template <typename Predicate>
  void wait(std::unique_lock<yaclib::detail::Mutex>& lock, Predicate predicate) {
    YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
    auto* m = lock.release();
    std::unique_lock guard{m->GetImpl(), std::adopt_lock};
    YACLIB_INJECT_FAULT(_impl.wait(guard, predicate));
    m->UpdateOwner(yaclib_std::this_thread::get_id());
    guard.release();
    lock = std::unique_lock{*m, std::adopt_lock};
  }

  template <typename Clock, typename Duration>
  std::cv_status wait_until(std::unique_lock<yaclib::detail::Mutex>& lock,
                            const std::chrono::time_point<Clock, Duration>& time_point) {
    YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
    auto* m = lock.release();
    std::unique_lock guard{m->GetImpl(), std::adopt_lock};
    YACLIB_INJECT_FAULT(auto result = _impl.wait_until(guard, time_point));
    m->UpdateOwner(yaclib_std::this_thread::get_id());
    guard.release();
    lock = std::unique_lock{*m, std::adopt_lock};
    return result;
  }

  template <typename Clock, typename Duration, typename Predicate>
  bool wait_until(std::unique_lock<yaclib::detail::Mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& time_point, Predicate predicate) {
    YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
    auto* m = lock.release();
    std::unique_lock guard{m->GetImpl(), std::adopt_lock};
    YACLIB_INJECT_FAULT(auto result = _impl.wait_until(guard, time_point, predicate));
    m->UpdateOwner(yaclib_std::this_thread::get_id());
    guard.release();
    lock = std::unique_lock{*m, std::adopt_lock};
    return result;
  }

  template <typename Rep, typename Period>
  std::cv_status wait_for(std::unique_lock<yaclib::detail::Mutex>& lock,
                          const std::chrono::duration<Rep, Period>& duration) {
    YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
    auto* m = lock.release();
    std::unique_lock guard{m->GetImpl(), std::adopt_lock};
    YACLIB_INJECT_FAULT(auto result = _impl.wait_for(guard, duration));
    m->UpdateOwner(yaclib_std::this_thread::get_id());
    guard.release();
    lock = std::unique_lock{*m, std::adopt_lock};
    return result;
  }

  template <typename Rep, typename Period, typename Predicate>
  bool wait_for(std::unique_lock<yaclib::detail::Mutex>& lock, const std::chrono::duration<Rep, Period>& duration,
                Predicate predicate) {
    YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
    auto* m = lock.release();
    std::unique_lock guard{m->GetImpl(), std::adopt_lock};
    YACLIB_INJECT_FAULT(auto result = _impl.wait_for(guard, duration, predicate));
    m->UpdateOwner(yaclib_std::this_thread::get_id());
    guard.release();
    lock = std::unique_lock{*m, std::adopt_lock};
    return result;
  }

  using native_handle_type = std::condition_variable::native_handle_type;

  native_handle_type native_handle();

 private:
  std::condition_variable _impl;
};

}  // namespace yaclib::detail

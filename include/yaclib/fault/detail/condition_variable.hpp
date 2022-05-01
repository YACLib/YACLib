#pragma once

#include <yaclib/fault/inject.hpp>
#include <yaclib/log.hpp>

#include <tuple>
#include <yaclib_std/chrono>
#include <yaclib_std/mutex>

namespace yaclib::detail {

// TODO(myannyax) unite with ConditionVariableAny

template <typename Impl>
class ConditionVariable : private Impl {
 public:
  using Impl::Impl;
  using Impl::native_handle;

  void notify_one() noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_one());
  }
  void notify_all() noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_all());
  }

  void wait(std::unique_lock<yaclib_std::mutex>& lock) {
    auto [mutex, impl_lock] = From(lock);
    YACLIB_INJECT_FAULT(Impl::wait(impl_lock));
    lock = From(mutex, impl_lock);
  }

  template <typename Predicate>
  void wait(std::unique_lock<yaclib_std::mutex>& lock, Predicate&& stop_waiting) {
    auto [mutex, impl_lock] = From(lock);
    YACLIB_INJECT_FAULT(Impl::wait(impl_lock, std::forward<Predicate>(stop_waiting)));
    lock = From(mutex, impl_lock);
  }

  template <typename Rep, typename Period>
  std::cv_status wait_for(std::unique_lock<yaclib_std::mutex>& lock,
                          const std::chrono::duration<Rep, Period>& rel_time) {
    auto [mutex, impl_lock] = From(lock);
    YACLIB_INJECT_FAULT(auto r = Impl::wait_for(impl_lock, rel_time));
    lock = From(mutex, impl_lock);
    return r;
  }
  template <typename Rep, typename Period, typename Predicate>
  bool wait_for(std::unique_lock<yaclib_std::mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time,
                Predicate&& stop_waiting) {
    auto [mutex, impl_lock] = From(lock);
    YACLIB_INJECT_FAULT(auto r = Impl::wait_for(impl_lock, rel_time, std::forward<Predicate>(stop_waiting)));
    lock = From(mutex, impl_lock);
    return r;
  }

  template <typename Clock, typename Duration>
  std::cv_status wait_until(std::unique_lock<yaclib_std::mutex>& lock,
                            const std::chrono::time_point<Clock, Duration>& timeout_time) {
    auto [mutex, impl_lock] = From(lock);
    YACLIB_INJECT_FAULT(auto r = Impl::wait_until(impl_lock, timeout_time));
    lock = From(mutex, impl_lock);
    return r;
  }
  template <typename Clock, typename Duration, typename Predicate>
  bool wait_until(std::unique_lock<yaclib_std::mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& timeout_time, Predicate&& stop_waiting) {
    auto [mutex, impl_lock] = From(lock);
    YACLIB_INJECT_FAULT(auto r = Impl::wait_until(impl_lock, timeout_time, std::forward<Predicate>(stop_waiting)));
    lock = From(mutex, impl_lock);
    return r;
  }

 private:
  static auto From(std::unique_lock<yaclib_std::mutex>& lock) {
    YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
    auto* mutex = lock.release();
    return std::tuple{mutex, std::unique_lock{mutex->GetImpl(), std::adopt_lock}};
  }
  static auto From(yaclib_std::mutex* mutex, std::unique_lock<yaclib_std::mutex::impl_t>& lock_impl) {
    YACLIB_ERROR(!lock_impl.owns_lock(), "After call wait on not owned lock");
    std::ignore = lock_impl.release();
    return std::unique_lock{*mutex, std::adopt_lock};
  }
};

}  // namespace yaclib::detail

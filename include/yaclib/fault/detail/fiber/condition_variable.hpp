#pragma once

#include <yaclib/fault/detail/fiber/mutex.hpp>

#include <condition_variable>

namespace yaclib::detail::fiber {

class ConditionVariable {
 public:
  ConditionVariable() noexcept = default;

  ~ConditionVariable() = default;

  ConditionVariable(const ConditionVariable&) = delete;
  ConditionVariable& operator=(const ConditionVariable&) = delete;

  void notify_one() noexcept;
  void notify_all() noexcept;

  void wait(std::unique_lock<yaclib::detail::fiber::Mutex>& lock) noexcept;

  template <typename Predicate>
  void wait(std::unique_lock<yaclib::detail::fiber::Mutex>& lock, Predicate predicate) {
    WaitImplWithPredicate(lock, NoTimeoutTag{}, predicate);
  }

  template <typename Clock, typename Duration>
  WaitStatus wait_until(std::unique_lock<yaclib::detail::fiber::Mutex>& lock,
                        const std::chrono::time_point<Clock, Duration>& time_point) {
    return WaitImpl(lock, time_point);
  }

  template <typename Clock, typename Duration, typename Predicate>
  bool wait_until(std::unique_lock<yaclib::detail::fiber::Mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& time_point, Predicate predicate) {
    return WaitImplWithPredicate(lock, time_point, predicate);
  }

  template <typename Rep, typename Period>
  WaitStatus wait_for(std::unique_lock<yaclib::detail::fiber::Mutex>& lock,
                      const std::chrono::duration<Rep, Period>& duration) {
    return WaitImpl(lock, duration);
  }

  template <typename Rep, typename Period, typename Predicate>
  bool wait_for(std::unique_lock<yaclib::detail::fiber::Mutex>& lock,
                const std::chrono::duration<Rep, Period>& duration, Predicate predicate) {
    return WaitImplWithPredicate(lock, duration, predicate);
  }

  using native_handle_type = std::condition_variable::native_handle_type;

  native_handle_type native_handle();

 private:
  template <typename Timeout, typename Predicate>
  bool WaitImplWithPredicate(std::unique_lock<yaclib::detail::fiber::Mutex>& lock, const Timeout& timeout,
                             const Predicate& predicate) {
    while (!predicate()) {
      if (WaitImpl(lock, timeout) == WaitStatus::Timeout) {
        break;
      }
    }
    if constexpr (!std::is_same_v<Timeout, NoTimeoutTag>) {
      return predicate();
    } else {
      YACLIB_DEBUG(!predicate(), "Should be true");
      return true;
    }
  }

  template <typename Timeout>
  WaitStatus WaitImpl(std::unique_lock<yaclib::detail::fiber::Mutex>& lock, const Timeout& timeout) {
    InjectFault();
    lock.unlock();
    auto status = _queue.Wait(timeout);
    lock.lock();
    InjectFault();
    return status;
  }

  FiberQueue _queue;
};

}  // namespace yaclib::detail::fiber

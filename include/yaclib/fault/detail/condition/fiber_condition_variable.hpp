#pragma once

#include <yaclib/fault/detail/mutex/fiber_mutex.hpp>

#include <condition_variable>

namespace yaclib::detail {

class FiberConditionVariable {
 public:
  FiberConditionVariable() noexcept = default;

  ~FiberConditionVariable() = default;

  FiberConditionVariable(const FiberConditionVariable&) = delete;
  FiberConditionVariable& operator=(const FiberConditionVariable&) = delete;

  void notify_one() noexcept;
  void notify_all() noexcept;

  void wait(std::unique_lock<yaclib::detail::FiberMutex>& lock) noexcept;

  template <typename Predicate>
  void wait(std::unique_lock<yaclib::detail::FiberMutex>& lock, Predicate predicate) {
    while (!predicate()) {
      lock.unlock();
      _queue.Wait();
      lock.lock();
    }
  }

  template <typename Clock, typename Duration>
  std::cv_status wait_until(std::unique_lock<yaclib::detail::FiberMutex>& lock,
                            const std::chrono::time_point<Clock, Duration>& time_point) {
    lock.unlock();
    _queue.Wait(time_point);
    lock.lock();
    return std::cv_status::no_timeout;
  }

  template <typename Clock, typename Duration, typename Predicate>
  bool wait_until(std::unique_lock<yaclib::detail::FiberMutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& time_point, Predicate predicate) {
    lock.unlock();
    _queue.Wait(time_point);
    lock.lock();
    return predicate();
  }

  template <typename Rep, typename Period>
  std::cv_status wait_for(std::unique_lock<yaclib::detail::FiberMutex>& lock,
                          const std::chrono::duration<Rep, Period>& duration) {
    lock.unlock();
    _queue.Wait(duration);
    lock.lock();
    return std::cv_status::no_timeout;
  }

  template <typename Rep, typename Period, typename Predicate>
  bool wait_for(std::unique_lock<yaclib::detail::FiberMutex>& lock, const std::chrono::duration<Rep, Period>& duration,
                Predicate predicate) {
    lock.unlock();
    _queue.Wait(duration);
    lock.lock();
    return predicate();
  }

  using native_handle_type = std::condition_variable::native_handle_type;

  native_handle_type native_handle();

 private:
  FiberQueue _queue;
};

}  // namespace yaclib::detail

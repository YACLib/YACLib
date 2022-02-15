#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/inject_fault.hpp>
#include <yaclib/log_config.hpp>

#include <atomic>
#include <mutex>

namespace yaclib::detail {

class TimedMutex {
 public:
  TimedMutex() = default;
  ~TimedMutex() noexcept = default;

  TimedMutex(const TimedMutex&) = delete;
  TimedMutex& operator=(const TimedMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  template <typename _Rep, typename _Period>
  bool try_lock_for(const std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(yaclib_std::chrono::steady_clock::now() + duration);
  }
  template <typename _Clock, typename _Duration>
  bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& duration) {
    YACLIB_INJECT_FAULT(auto res = _m.try_lock_until(duration));

    if (res) {
      _owner = yaclib_std::this_thread::get_id();
    }
    return res;
  }

 private:
  std::timed_mutex _m;
  // TODO(myannyax) yaclib wrapper
  yaclib_std::thread::id _owner = yaclib::detail::kInvalidThreadId;
};

}  // namespace yaclib::detail

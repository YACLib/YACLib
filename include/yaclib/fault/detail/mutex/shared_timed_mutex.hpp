#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/inject_fault.hpp>
#include <yaclib/log.hpp>

#include <shared_mutex>

namespace yaclib::detail {

class SharedTimedMutex {
 public:
  SharedTimedMutex() = default;
  ~SharedTimedMutex() noexcept = default;

  SharedTimedMutex(const SharedTimedMutex&) = delete;
  SharedTimedMutex& operator=(const SharedTimedMutex&) = delete;

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
    return res;
  }

  void lock_shared();
  bool try_lock_shared() noexcept;
  void unlock_shared() noexcept;

  template <typename _Rep, typename _Period>
  bool try_lock_shared_for(const std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(yaclib_std::chrono::steady_clock::now() + duration);
  }
  template <typename _Clock, typename _Duration>
  bool try_lock_shared_until(const std::chrono::time_point<_Clock, _Duration>& duration) {
    YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared_until(duration));
    return res;
  }

 private:
  std::shared_timed_mutex _m;
};

}  // namespace yaclib::detail

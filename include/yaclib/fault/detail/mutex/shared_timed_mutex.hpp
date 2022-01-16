#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/antagonist/inject_fault.hpp>
#include <yaclib/fault/log_config.hpp>

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
    auto me = yaclib_std::this_thread::get_id();
    Log(_exclusive_owner != me, "trying to lock owned mutex with non-recursive lock");

    YACLIB_INJECT_FAULT(auto res = _m.try_lock_until(duration));

    if (res) {
      _exclusive_owner = yaclib_std::this_thread::get_id();
      _shared_mode.store(false);
    }
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
    auto me = yaclib_std::this_thread::get_id();
    Log(_exclusive_owner != me, "trying to lock_shared mutex that is already owned in exclusive mode");

    YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared_until(duration));

    if (res) {
      _shared_mode.store(true);
    }
    return res;
  }

 private:
  std::shared_timed_mutex _m;
  // TODO(myannyax) yaclib wrapper
  std::atomic<yaclib_std::thread::id> _exclusive_owner{yaclib::detail::kInvalidThreadId};
  std::atomic_bool _shared_mode{false};
};

}  // namespace yaclib::detail

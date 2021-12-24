#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <cassert>
#include <shared_mutex>
#include <unordered_set>

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
    assert(_exclusive_owner != me);
    {
      std::shared_lock lock(_helper_m);
      assert(_shared_owners.find(me) == _shared_owners.end());
    }

    YACLIB_INJECT_FAULT(auto res = _m.try_lock_until(duration);)

    if (res) {
      _exclusive_owner = yaclib_std::this_thread::get_id();
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
    assert(_exclusive_owner != me);
    {
      std::shared_lock lock(_helper_m);
      assert(_shared_owners.find(me) == _shared_owners.end());
    }

    YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared_until(duration);)

    if (res) {
      {
        std::unique_lock lock(_helper_m);
        _shared_owners.insert(me);
      }
    }
    return res;
  }

 private:
  std::shared_timed_mutex _m;
  std::shared_mutex _helper_m;  // for _shared_owners

  // TODO(myannyax) yaclib wrapper
  std::atomic<yaclib_std::thread::id> _exclusive_owner{yaclib::detail::kInvalidThreadId};
  // TODO(myannyax) remove / change?
  std::unordered_set<yaclib_std::thread::id> _shared_owners;
};

}  // namespace yaclib::detail

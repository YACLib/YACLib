#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

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
  bool try_lock_for(const ::std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(yaclib::std::chrono::steady_clock::now() + duration);
  }
  template <typename _Clock, typename _Duration>
  bool try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration);

  void lock_shared();
  bool try_lock_shared() noexcept;
  void unlock_shared() noexcept;

  template <typename _Rep, typename _Period>
  bool try_lock_shared_for(const ::std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(yaclib::std::chrono::steady_clock::now() + duration);
  }
  template <typename _Clock, typename _Duration>
  bool try_lock_shared_until(const ::std::chrono::time_point<_Clock, _Duration>& duration);

 private:
#if defined(YACLIB_FIBER)
  kek _m;
#else
  ::std::shared_timed_mutex _m;
  ::std::shared_mutex _helper_m;  // for _shared_owners
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _exclusive_owner{yaclib::detail::kInvalidThreadId};
  // TODO(myannyax) remove / change?
  ::std::unordered_set<yaclib::std::thread::id> _shared_owners;
};

}  // namespace yaclib::detail

#pragma once
#include <yaclib/fault_injection/antagonist/inject_fault.hpp>
#include <yaclib/fault_injection/chrono/steady_clock.hpp>

#include <shared_mutex>
#include <unordered_set>

namespace yaclib::std {

#if defined(YACLIB_FAULTY)

namespace detail {

class SharedTimedMutex {
 public:
  SharedTimedMutex() = default;
  ~SharedTimedMutex() noexcept = default;

  SharedTimedMutex(const SharedTimedMutex&) = delete;
  SharedTimedMutex& operator=(const SharedTimedMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  template <class _Rep, class _Period>
  bool try_lock_for(const ::std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(chrono::steady_clock::now() + duration);
  }
  template <class _Clock, class _Duration>
  bool try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration);

  void lock_shared();
  bool try_lock_shared() noexcept;
  void unlock_shared() noexcept;

  template <class _Rep, class _Period>
  bool try_lock_shared_for(const ::std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(chrono::steady_clock::now() + duration);
  }
  template <class _Clock, class _Duration>
  bool try_lock_shared_until(const ::std::chrono::time_point<_Clock, _Duration>& duration);

 private:
#if defined(YACLIB_FIBER)
  kek _m;
#else
  ::std::shared_timed_mutex _m;
  ::std::shared_mutex _helper_m;  // for _shared_owners
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _exclusive_owner{yaclib::std::detail::kInvalidThreadId};
  // TODO(myannyax) remove / change?
  ::std::unordered_set<yaclib::std::thread::id> _shared_owners;
};

class SharedMutex {
 public:
  SharedMutex() = default;
  ~SharedMutex() noexcept = default;

  SharedMutex(const SharedMutex&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  void lock_shared();
  bool try_lock_shared() noexcept;
  void unlock_shared() noexcept;

  // TODO(myannyax) no handle?

 private:
#if defined(YACLIB_FIBER)
  kek _m;
#else
  ::std::shared_mutex _m;
  ::std::shared_mutex _helper_m;  // for _shared_owners
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _exclusive_owner{yaclib::std::detail::kInvalidThreadId};
  // TODO(myannyax) remove / change?
  ::std::unordered_set<yaclib::std::thread::id> _shared_owners;
};
}  // namespace detail

using shared_mutex = detail::SharedMutex;
using shared_timed_mutex = detail::SharedTimedMutex;

#else

using shared_mutex = ::std::shared_mutex;
using shared_timed_mutex = ::std::shared_timed_mutex;

#endif
}  // namespace yaclib::std

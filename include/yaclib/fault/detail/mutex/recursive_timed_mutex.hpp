#pragma once

#include <yaclib/fault/chrono.hpp>
#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <mutex>

namespace yaclib::detail {

class RecursiveTimedMutex {
 public:
  RecursiveTimedMutex() = default;
  ~RecursiveTimedMutex() noexcept = default;

  RecursiveTimedMutex(const RecursiveTimedMutex&) = delete;
  RecursiveTimedMutex& operator=(const RecursiveTimedMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  template <class _Rep, class _Period>
  bool try_lock_for(const ::std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(yaclib::std::chrono::steady_clock::now() + duration);
  }
  template <class _Clock, class _Duration>
  bool try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration);

 private:
#if defined(YACLIB_FIBER)
  kek _m;
#else
  ::std::recursive_timed_mutex _m;
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::detail::kInvalidThreadId};
  ::std::atomic<unsigned> _lock_level{0};
};

}  // namespace yaclib::detail

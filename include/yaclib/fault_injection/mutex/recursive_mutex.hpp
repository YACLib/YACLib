#pragma once
#include <yaclib/fault_injection/antagonist/inject_fault.hpp>
#include <yaclib/fault_injection/chrono/steady_clock.hpp>

#include <mutex>

namespace yaclib::std {

#if defined(YACLIB_FAULTY)

namespace detail {
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
    return try_lock_until(chrono::steady_clock::now() + duration);
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
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::std::detail::kInvalidThreadId};
  ::std::atomic<unsigned> _lock_level{0};
};

class RecursiveMutex {
 public:
  RecursiveMutex() = default;
  ~RecursiveMutex() noexcept = default;

  RecursiveMutex(const RecursiveMutex&) = delete;
  RecursiveMutex& operator=(const RecursiveMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

#if defined(YACLIB_FIBER)
  // TODO(myannyax)
  using native_handle_type = kek;
#else
  using native_handle_type = ::std::recursive_mutex::native_handle_type;
#endif

  native_handle_type native_handle();

 private:
#if defined(YACLIB_FIBER)
  kek _m;
#else
  ::std::recursive_mutex _m;
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::std::detail::kInvalidThreadId};
  ::std::atomic<unsigned> _lock_level{0};
};
}  // namespace detail

using recursive_timed_mutex = detail::RecursiveTimedMutex;
using recursive_mutex = detail::RecursiveMutex;

#else

using recursive_timed_mutex = ::std::recursive_timed_mutex;
using recursive_mutex = ::std::recursive_mutex;

#endif

}  // namespace yaclib::std

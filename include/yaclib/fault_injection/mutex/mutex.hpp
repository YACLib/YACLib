#pragma once
#include <yaclib/fault_injection/antagonist/inject_fault.hpp>
#include <yaclib/fault_injection/chrono/steady_clock.hpp>

#include <atomic>
#include <mutex>

namespace yaclib::std {

#if defined(YACLIB_FAULTY)

namespace detail {

class TimedMutex {
 public:
  TimedMutex() = default;
  ~TimedMutex() noexcept = default;

  TimedMutex(const TimedMutex&) = delete;
  TimedMutex& operator=(const TimedMutex&) = delete;

  inline void lock();
  inline bool try_lock() noexcept;
  inline void unlock() noexcept;

  template <class _Rep, class _Period>
  inline bool try_lock_for(const ::std::chrono::duration<_Rep, _Period>& duration) {
    return try_lock_until(chrono::steady_clock::now() + duration);
  }
  template <class _Clock, class _Duration>
  inline bool try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration);

 private:
#if defined(YACLIB_FIBER)
  // TODO(myannyax)
  kek _m;
#else
  ::std::timed_mutex _m;
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::std::detail::kInvalidThreadId};
};

class Mutex {
 public:
  Mutex() = default;
  ~Mutex() noexcept = default;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  inline void lock();
  inline bool try_lock() noexcept;
  inline void unlock() noexcept;

#if defined(YACLIB_FIBER)
  // TODO(myannyax)
  using native_handle_type = kek;
#else
  using native_handle_type = ::std::mutex::native_handle_type;
#endif

  inline native_handle_type native_handle();

 private:
#if defined(YACLIB_FIBER)
  kek _m;
#else
  ::std::mutex _m;
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::std::detail::kInvalidThreadId};
};

}  // namespace detail

using mutex = detail::Mutex;
using timed_mutex = detail::TimedMutex;

#else

using mutex = ::std::mutex;
using timed_mutex = ::std::timed_mutex;

#endif
}  // namespace yaclib::std

#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <atomic>
#include <mutex>

namespace yaclib::detail {

class Mutex {
 public:
  Mutex() = default;
  ~Mutex() noexcept = default;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

#ifdef YACLIB_FIBER
  // TODO(myannyax)
  using native_handle_type = kek;
#else
  using native_handle_type = ::std::mutex::native_handle_type;
#endif

  inline native_handle_type native_handle();

 private:
#ifdef YACLIB_FIBER
  kek _m;
#else
  ::std::mutex _m;
#endif
  // TODO(myannyax) yaclib wrapper
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::detail::kInvalidThreadId};
};

}  // namespace yaclib::detail

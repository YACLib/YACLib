#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <mutex>

namespace yaclib::detail {

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
  ::std::atomic<yaclib::std::thread::id> _owner{yaclib::detail::kInvalidThreadId};
  ::std::atomic<unsigned> _lock_level{0};
};

}  // namespace yaclib::detail

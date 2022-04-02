#include <yaclib/config.hpp>
#include <yaclib/fault/detail/mutex/recursive_mutex.hpp>

// TODO(myannyax) avoid copypaste from recursive_timed_mutex
namespace yaclib::detail {

void RecursiveMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());
}

bool RecursiveMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());
  return res;
}

void RecursiveMutex::unlock() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock());
}

RecursiveMutex::native_handle_type RecursiveMutex::native_handle() {
  return _m.native_handle();
}

}  // namespace yaclib::detail

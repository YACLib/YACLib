#include <yaclib/fault/detail/mutex/timed_mutex.hpp>

namespace yaclib::detail {

void TimedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());

  _owner = yaclib_std::this_thread::get_id();
}

bool TimedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());

  if (res) {
    _owner = yaclib_std::this_thread::get_id();
  }
  return res;
}

void TimedMutex::unlock() noexcept {
  YACLIB_ERROR(_owner == yaclib::detail::kInvalidThreadId, "trying to unlock not locked mutex");
  YACLIB_ERROR(_owner != yaclib_std::this_thread::get_id(), "trying to unlock mutex that's not owned by this thread");

  YACLIB_INJECT_FAULT(_m.unlock());

  _owner = yaclib::detail::kInvalidThreadId;
}

}  // namespace yaclib::detail

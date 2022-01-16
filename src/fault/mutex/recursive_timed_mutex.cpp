#include <yaclib/fault/detail/mutex/recursive_timed_mutex.hpp>

namespace yaclib::detail {

void RecursiveTimedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());

  UpdateOnLock();
}

bool RecursiveTimedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());

  if (res) {
    UpdateOnLock();
  }
  return res;
}

void RecursiveTimedMutex::unlock() noexcept {
  Log(_owner != yaclib::detail::kInvalidThreadId, "trying to unlock not locked mutex");
  Log(_owner == yaclib_std::this_thread::get_id(), "trying to unlock mutex that's not owned by this thread");

  _lock_level--;
  if (_lock_level == 0) {
    _owner = yaclib::detail::kInvalidThreadId;
  }

  YACLIB_INJECT_FAULT(_m.unlock());
}

void RecursiveTimedMutex::UpdateOnLock() {
  ++_lock_level;
  _owner = yaclib_std::this_thread::get_id();
}

}  // namespace yaclib::detail

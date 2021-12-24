#include <yaclib/fault/detail/mutex/recursive_timed_mutex.hpp>

namespace yaclib::detail {

void RecursiveTimedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock();)

  UpdateOnLock();
}

bool RecursiveTimedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock();)

  if (res) {
    UpdateOnLock();
  }
  return res;
}

void RecursiveTimedMutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib_std::this_thread::get_id());

  _lock_level--;
  if (_lock_level == 0) {
    _owner = yaclib::detail::kInvalidThreadId;
  }

  YACLIB_INJECT_FAULT(_m.unlock();)
}

void RecursiveTimedMutex::UpdateOnLock() {
  if (_owner == yaclib_std::this_thread::get_id()) {
    assert(_lock_level > 0);
  } else {
    assert(_lock_level == 0);
  }
  ++_lock_level;
  _owner = yaclib_std::this_thread::get_id();
}

}  // namespace yaclib::detail

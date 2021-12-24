#include <yaclib/fault/detail/mutex/recursive_mutex.hpp>

// TODO(myannyax) avoid copypaste from recursive_timed_mutex
namespace yaclib::detail {

void RecursiveMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock();)

  UpdateOnLock();
}

bool RecursiveMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock();)

  if (res) {
    UpdateOnLock();
  }
  return res;
}

void RecursiveMutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib_std::this_thread::get_id());

  _lock_level--;
  if (_lock_level == 0) {
    _owner = yaclib::detail::kInvalidThreadId;
  }

  YACLIB_INJECT_FAULT(_m.unlock();)
}

RecursiveMutex::native_handle_type RecursiveMutex::native_handle() {
  return _m.native_handle();
}

void RecursiveMutex::UpdateOnLock() {
  if (_owner == yaclib_std::this_thread::get_id()) {
    assert(_lock_level > 0);
  } else {
    assert(_owner == yaclib::detail::kInvalidThreadId);
    assert(_lock_level == 0);
  }
  ++_lock_level;
  _owner = yaclib_std::this_thread::get_id();
}

}  // namespace yaclib::detail

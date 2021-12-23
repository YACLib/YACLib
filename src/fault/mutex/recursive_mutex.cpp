#include <yaclib/fault/detail/mutex/recursive_mutex.hpp>

// TODO(myannyax) avoid copypaste from recursive_timed_mutex
namespace yaclib::detail {

void RecursiveMutex::lock() {
  yaclib::detail::InjectFault();
  _m.lock();
  yaclib::detail::InjectFault();

  UpdateOnLock();
}

bool RecursiveMutex::try_lock() noexcept {
  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  yaclib::detail::InjectFault();

  if (res) {
    UpdateOnLock();
  }
  return res;
}

void RecursiveMutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib::std::this_thread::get_id());

  _lock_level--;
  if (_lock_level == 0) {
    _owner = yaclib::detail::kInvalidThreadId;
  }

  yaclib::detail::InjectFault();
  _m.unlock();
  yaclib::detail::InjectFault();
}

RecursiveMutex::native_handle_type RecursiveMutex::native_handle() {
  return _m.native_handle();
}

void RecursiveMutex::UpdateOnLock() {
  unsigned new_level = _lock_level;
  if (_owner == yaclib::std::this_thread::get_id()) {
    new_level++;
  } else {
    new_level = 1;
  }
  _lock_level = new_level;
  _owner = yaclib::std::this_thread::get_id();
}

}  // namespace yaclib::detail

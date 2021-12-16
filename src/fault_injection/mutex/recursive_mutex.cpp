#include <yaclib/fault_injection/mutex/recursive_mutex.hpp>

// TODO(myannyax) avoid copypaste from recursive_timed_mutex
namespace yaclib::std::detail {

void RecursiveMutex::lock() {
  unsigned new_level = _lock_level;
  if (_owner == yaclib::std::this_thread::get_id()) {
    new_level++;
  } else {
    new_level = 1;
  }
  yaclib::detail::InjectFault();
  _m.lock();
  _lock_level = new_level;
  _owner = yaclib::std::this_thread::get_id();
  yaclib::detail::InjectFault();
}

bool RecursiveMutex::try_lock() noexcept {
  unsigned new_level = _lock_level;
  if (_owner == yaclib::std::this_thread::get_id()) {
    new_level++;
  } else {
    new_level = 1;
  }
  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  if (res) {
    _lock_level = new_level;
    _owner = yaclib::std::this_thread::get_id();
  }
  yaclib::detail::InjectFault();
  return res;
}

void RecursiveMutex::unlock() noexcept {
  assert(_owner != yaclib::std::detail::kInvalidThreadId);
  assert(_owner == yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.unlock();
  _lock_level--;
  if (_lock_level == 0) {
    _owner = yaclib::std::detail::kInvalidThreadId;
  }
  yaclib::detail::InjectFault();
}

RecursiveMutex::native_handle_type RecursiveMutex::native_handle() {
  return _m.native_handle();
}

}  // namespace yaclib::std::detail

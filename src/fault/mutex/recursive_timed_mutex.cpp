#include <yaclib/fault/detail/mutex/recursive_timed_mutex.hpp>

namespace yaclib::detail {

void RecursiveTimedMutex::lock() {
  yaclib::detail::InjectFault();
  _m.lock();
  yaclib::detail::InjectFault();

  UpdateOnLock();
}

bool RecursiveTimedMutex::try_lock() noexcept {
  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  yaclib::detail::InjectFault();

  if (res) {
    UpdateOnLock();
  }
  return res;
}

void RecursiveTimedMutex::unlock() noexcept {
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

template <typename _Clock, typename _Duration>
bool RecursiveTimedMutex::try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  yaclib::detail::InjectFault();
  auto res = _m.try_lock_until(duration);
  yaclib::detail::InjectFault();

  if (res) {
    UpdateOnLock();
  }
  return res;
}

void RecursiveTimedMutex::UpdateOnLock() {
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

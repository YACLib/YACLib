#include <yaclib/fault/detail/mutex/timed_mutex.hpp>

namespace yaclib::detail {

void TimedMutex::lock() {
  assert(_owner != yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.lock();
  _owner = yaclib::std::this_thread::get_id();
  yaclib::detail::InjectFault();
}

bool TimedMutex::try_lock() noexcept {
  assert(_owner != yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  if (res) {
    _owner = yaclib::std::this_thread::get_id();
  }
  yaclib::detail::InjectFault();
  return res;
}

void TimedMutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.unlock();
  _owner = yaclib::detail::kInvalidThreadId;
  yaclib::detail::InjectFault();
}

template <class _Clock, class _Duration>
bool TimedMutex::try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  assert(_owner != yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  auto res = _m.try_lock_until(duration);
  if (res) {
    _owner = yaclib::std::this_thread::get_id();
  }
  yaclib::detail::InjectFault();
  return res;
}

}  // namespace yaclib::detail

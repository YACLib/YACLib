#include <yaclib/fault/detail/mutex/timed_mutex.hpp>

namespace yaclib::detail {

void TimedMutex::lock() {
  assert(_owner != yaclib::std::this_thread::get_id());

  yaclib::detail::InjectFault();
  _m.lock();
  yaclib::detail::InjectFault();

  _owner = yaclib::std::this_thread::get_id();
}

bool TimedMutex::try_lock() noexcept {
  assert(_owner != yaclib::std::this_thread::get_id());

  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  yaclib::detail::InjectFault();

  if (res) {
    _owner = yaclib::std::this_thread::get_id();
  }
  return res;
}

void TimedMutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib::std::this_thread::get_id());

  yaclib::detail::InjectFault();
  _m.unlock();
  yaclib::detail::InjectFault();

  _owner = yaclib::detail::kInvalidThreadId;
}

template <typename _Clock, typename _Duration>
bool TimedMutex::try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  assert(_owner != yaclib::std::this_thread::get_id());

  yaclib::detail::InjectFault();
  auto res = _m.try_lock_until(duration);
  yaclib::detail::InjectFault();

  if (res) {
    _owner = yaclib::std::this_thread::get_id();
  }
  return res;
}

}  // namespace yaclib::detail

#include <yaclib/fault/detail/mutex/timed_mutex.hpp>

namespace yaclib::detail {

void TimedMutex::lock() {
  assert(_owner != yaclib_std::this_thread::get_id());

  YACLIB_INJECT_FAULT(_m.lock());

  _owner = yaclib_std::this_thread::get_id();
}

bool TimedMutex::try_lock() noexcept {
  assert(_owner != yaclib_std::this_thread::get_id());

  YACLIB_INJECT_FAULT(auto res = _m.try_lock());

  if (res) {
    _owner = yaclib_std::this_thread::get_id();
  }
  return res;
}

void TimedMutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib_std::this_thread::get_id());

  YACLIB_INJECT_FAULT(_m.unlock());

  _owner = yaclib::detail::kInvalidThreadId;
}

}  // namespace yaclib::detail

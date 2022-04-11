
#include <yaclib/fault/detail/mutex/mutex.hpp>

// TODO(myannyax) avoid copypaste from timed_mutex
namespace yaclib::detail {

void Mutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());
}

bool Mutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());
  return res;
}

void Mutex::unlock() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock());
}

Mutex::native_handle_type Mutex::native_handle() {
  return _m.native_handle();
}

Mutex::impl_type& Mutex::GetImpl() {
  return _m;
}

}  // namespace yaclib::detail

#include <yaclib/fault/detail/mutex/mutex.hpp>

// TODO(myannyax) avoid copypaste from timed_mutex
namespace yaclib::detail {

void Mutex::lock() {
  Log(_owner != yaclib_std::this_thread::get_id(), "trying to lock owned mutex with non-recursive lock");

  YACLIB_INJECT_FAULT(_m.lock());

  _owner = yaclib_std::this_thread::get_id();
}

bool Mutex::try_lock() noexcept {
  Log(_owner != yaclib_std::this_thread::get_id(), "trying to lock owned mutex with non-recursive lock");

  YACLIB_INJECT_FAULT(auto res = _m.try_lock());

  if (res) {
    _owner = yaclib_std::this_thread::get_id();
  }
  return res;
}

void Mutex::unlock() noexcept {
  Log(_owner != yaclib::detail::kInvalidThreadId, "trying to unlock not locked mutex");
  Log(_owner == yaclib_std::this_thread::get_id(), "trying to unlock mutex that's not owned by this thread");

  _owner = yaclib::detail::kInvalidThreadId;

  YACLIB_INJECT_FAULT(_m.unlock());
}

Mutex::native_handle_type Mutex::native_handle() {
  return _m.native_handle();
}

Mutex::impl_type& Mutex::GetImpl() {
  return _m;
}

void Mutex::UpdateOwner(yaclib_std::thread::id new_owner) {
  _owner = new_owner;
}

}  // namespace yaclib::detail

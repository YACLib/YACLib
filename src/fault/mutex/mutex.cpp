#include <yaclib/fault/detail/mutex/mutex.hpp>

// TODO(myannyax) avoid copypaste from timed_mutex
namespace yaclib::detail {

void Mutex::lock() {
  assert(_owner != yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.lock();
  _owner = yaclib::std::this_thread::get_id();
  yaclib::detail::InjectFault();
}

bool Mutex::try_lock() noexcept {
  assert(_owner != yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  if (res) {
    _owner = yaclib::std::this_thread::get_id();
  }
  yaclib::detail::InjectFault();
  return res;
}

void Mutex::unlock() noexcept {
  assert(_owner != yaclib::detail::kInvalidThreadId);
  assert(_owner == yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.unlock();
  _owner = yaclib::detail::kInvalidThreadId;
  yaclib::detail::InjectFault();
}

Mutex::native_handle_type Mutex::native_handle() {
  return _m.native_handle();
}

}  // namespace yaclib::detail

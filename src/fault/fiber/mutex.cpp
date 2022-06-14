#include <yaclib/fault/detail/fiber/mutex.hpp>

namespace yaclib::detail::fiber {

void Mutex::lock() {
  while (_occupied) {
    _queue.Wait(NoTimeoutTag{});
  }
  _occupied = true;
}

bool Mutex::try_lock() noexcept {
  if (_occupied) {
    return false;
  }
  _occupied = true;
  return true;
}

void Mutex::unlock() noexcept {
  _occupied = false;
  _queue.NotifyOne();
}

Mutex::native_handle_type Mutex::native_handle() {
  return nullptr;
}

}  // namespace yaclib::detail::fiber

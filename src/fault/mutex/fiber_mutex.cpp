#include <yaclib/fault/detail/mutex/fiber_mutex.hpp>

namespace yaclib::detail {

void FiberMutex::lock() {
  InjectFault();
  while (_occupied) {
    _queue.Wait();
  }
  _occupied = true;
  InjectFault();
}

bool FiberMutex::try_lock() noexcept {
  InjectFault();
  if (_occupied) {
    return false;
  }
  _occupied = true;
  InjectFault();
  return true;
}

void FiberMutex::unlock() noexcept {
  InjectFault();
  _occupied = false;
  _queue.NotifyOne();
  InjectFault();
}

FiberMutex::native_handle_type FiberMutex::native_handle() {
  return nullptr;
}

void FiberMutex::LockNoInject() {
  while (_occupied) {
    _queue.Wait();
  }
  _occupied = true;
}

void FiberMutex::UnlockNoInject() noexcept {
  _occupied = false;
  _queue.NotifyOne();
}
}  // namespace yaclib::detail

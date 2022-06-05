#include <fault/util.hpp>

#include <yaclib/fault/detail/fiber/shared_mutex.hpp>

namespace yaclib::detail::fiber {

void fiber::SharedMutex::lock() {
  if (_occupied) {
    _exclusive_queue.Wait(NoTimeoutTag{});
  }
  LockHelper();
}

bool SharedMutex::try_lock() noexcept {
  if (_occupied) {
    return false;
  }
  LockHelper();
  return true;
}

void SharedMutex::unlock() noexcept {
  const bool unlock_shared = !_shared_queue.Empty() && (_exclusive_queue.Empty() || GetRandNumber(2) == 0);
  _occupied = false;
  if (unlock_shared) {
    _shared_queue.NotifyAll();
  } else {
    _exclusive_queue.NotifyOne();
  }
}

void SharedMutex::lock_shared() {
  if (_occupied && _exclusive_mode) {
    _exclusive_queue.Wait(NoTimeoutTag{});
  }
  SharedLockHelper();
}

bool SharedMutex::try_lock_shared() {
  if (_occupied && _exclusive_mode) {
    return false;
  }
  SharedLockHelper();
  return true;
}

void SharedMutex::unlock_shared() {
  _shared_owners_count--;
  if (_shared_owners_count == 0) {
    _occupied = false;
    _exclusive_queue.NotifyOne();
  }
}

void SharedMutex::LockHelper() {
  _occupied = true;
  _exclusive_mode = true;
}

void SharedMutex::SharedLockHelper() {
  _occupied = true;
  _exclusive_mode = false;
  _shared_owners_count++;
}

SharedMutex::native_handle_type SharedMutex::native_handle() {
  return nullptr;
}

}  // namespace yaclib::detail::fiber

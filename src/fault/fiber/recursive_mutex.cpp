
#include <yaclib/fault/detail/fiber/recursive_mutex.hpp>

namespace yaclib::detail::fiber {
void fiber::RecursiveMutex::lock() {
  if (_occupied_count != 0 && _owner_id != fault::Scheduler::GetId()) {
    _queue.Wait(NoTimeoutTag{});
  }
  LockHelper();
}

bool RecursiveMutex::try_lock() noexcept {
  if (_occupied_count != 0 && _owner_id != fault::Scheduler::GetId()) {
    return false;
  }
  LockHelper();
  return true;
}

void RecursiveMutex::unlock() noexcept {
  _occupied_count--;
  YACLIB_ERROR(_occupied_count < 0, "unlock on not locked recursive mutex");
  if (_occupied_count == 0) {
    _owner_id = 0;
  }
}
void RecursiveMutex::LockHelper() {
  _occupied_count++;
  _owner_id = fault::Scheduler::GetId();
}

RecursiveMutex::native_handle_type RecursiveMutex::native_handle() {
  return nullptr;
}
}  // namespace yaclib::detail::fiber

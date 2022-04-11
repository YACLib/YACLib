
#include <yaclib/fault/detail/mutex/shared_timed_mutex.hpp>

namespace yaclib::detail {

void SharedTimedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());
}

bool SharedTimedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());
  return res;
}

void SharedTimedMutex::unlock() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock());
}

void SharedTimedMutex::lock_shared() {
  YACLIB_INJECT_FAULT(_m.lock_shared());
}

bool SharedTimedMutex::try_lock_shared() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared());
  return res;
}

void SharedTimedMutex::unlock_shared() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock_shared());
}

}  // namespace yaclib::detail

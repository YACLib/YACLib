#include <yaclib/config.hpp>
#include <yaclib/fault/detail/mutex/shared_mutex.hpp>

// TODO(myannyax) avoid copypaste from shared_timed_mutex
namespace yaclib::detail {

void SharedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());
}

bool SharedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());
  return res;
}

void SharedMutex::unlock() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock());
}

void SharedMutex::lock_shared() {
  YACLIB_INJECT_FAULT(_m.lock_shared());
}

bool SharedMutex::try_lock_shared() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared());
  return res;
}

void SharedMutex::unlock_shared() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock_shared());
}

}  // namespace yaclib::detail

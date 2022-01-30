#include <yaclib/fault/detail/mutex/shared_mutex.hpp>

// TODO(myannyax) avoid copypaste from shared_timed_mutex
namespace yaclib::detail {

void SharedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());

  _exclusive_owner = yaclib_std::this_thread::get_id();
  _shared_mode = false;
}

bool SharedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());

  if (res) {
    _exclusive_owner = yaclib_std::this_thread::get_id();
    _shared_mode = false;
  }
  return res;
}

void SharedMutex::unlock() noexcept {
  YACLIB_ERROR(_exclusive_owner == yaclib::detail::kInvalidThreadId, "trying to unlock not locked mutex");
  YACLIB_ERROR(_exclusive_owner != yaclib_std::this_thread::get_id(),
               "trying to unlock mutex that's not owned by this thread");
  YACLIB_ERROR(_shared_mode == true, "trying to exclusively unlock mutex in shared mode");

  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  YACLIB_INJECT_FAULT(_m.unlock());
}

void SharedMutex::lock_shared() {
  YACLIB_INJECT_FAULT(_m.lock_shared());

  _shared_mode = true;
}

bool SharedMutex::try_lock_shared() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared());

  if (res) {
    _shared_mode = true;
  }
  return res;
}

void SharedMutex::unlock_shared() noexcept {
  YACLIB_ERROR(_shared_mode == false, "trying to unlock_shared mutex that is not in shared mode");

  YACLIB_INJECT_FAULT(_m.unlock_shared());
}

}  // namespace yaclib::detail

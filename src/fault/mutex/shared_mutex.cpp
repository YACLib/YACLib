#include <yaclib/fault/detail/mutex/shared_mutex.hpp>

// TODO(myannyax) avoid copypaste from shared_timed_mutex
namespace yaclib::detail {

void SharedMutex::lock() {
  auto me = yaclib_std::this_thread::get_id();
  Log(_exclusive_owner != me, "trying to lock owned mutex with non-recursive lock");

  YACLIB_INJECT_FAULT(_m.lock());

  _exclusive_owner = me;
  _shared_mode.store(false);
}

bool SharedMutex::try_lock() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  Log(_exclusive_owner != me, "trying to lock owned mutex with non-recursive lock");

  YACLIB_INJECT_FAULT(auto res = _m.try_lock());

  if (res) {
    _exclusive_owner = me;
    _shared_mode.store(false);
  }
  return res;
}

void SharedMutex::unlock() noexcept {
  Log(_exclusive_owner != yaclib::detail::kInvalidThreadId, "trying to unlock not locked mutex");
  Log(_exclusive_owner == yaclib_std::this_thread::get_id(), "trying to unlock mutex that's not owned by this thread");
  Log(_shared_mode == false, "trying to exclusively unlock mutex in shared mode");

  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  YACLIB_INJECT_FAULT(_m.unlock());
}

void SharedMutex::lock_shared() {
  auto me = yaclib_std::this_thread::get_id();
  Log(_exclusive_owner != me, "trying to lock_shared mutex that is already owned in exclusive mode");

  YACLIB_INJECT_FAULT(_m.lock_shared());

  _shared_mode.store(true);
}

bool SharedMutex::try_lock_shared() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  Log(_exclusive_owner != me, "trying to lock_shared mutex that is already owned in exclusive mode");

  YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared());

  if (res) {
    _shared_mode.store(true);
  }
  return res;
}

void SharedMutex::unlock_shared() noexcept {
  Log(_shared_mode = true, "trying to unlock_shared mutex that is not in shared mode");

  YACLIB_INJECT_FAULT(_m.unlock_shared());
}

}  // namespace yaclib::detail

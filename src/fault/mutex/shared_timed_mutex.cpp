#include <yaclib/fault/detail/mutex/shared_timed_mutex.hpp>

namespace yaclib::detail {

void SharedTimedMutex::lock() {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);

  YACLIB_INJECT_FAULT(_m.lock();)

  _exclusive_owner = me;
  _shared_mode.store(false);
}

bool SharedTimedMutex::try_lock() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);

  YACLIB_INJECT_FAULT(auto res = _m.try_lock();)

  if (res) {
    _exclusive_owner = yaclib_std::this_thread::get_id();
    _shared_mode.store(false);
  }
  return res;
}

void SharedTimedMutex::unlock() noexcept {
  assert(_exclusive_owner != yaclib::detail::kInvalidThreadId);
  assert(_exclusive_owner == yaclib_std::this_thread::get_id());
  assert(_shared_mode == false);

  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  YACLIB_INJECT_FAULT(_m.unlock();)
}

void SharedTimedMutex::lock_shared() {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);

  YACLIB_INJECT_FAULT(_m.lock_shared();)

  _shared_mode.store(true);
}

bool SharedTimedMutex::try_lock_shared() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);

  YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared();)

  if (res) {
    _shared_mode.store(true);
  }
  return res;
}

void SharedTimedMutex::unlock_shared() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  assert(_shared_mode = true);

  YACLIB_INJECT_FAULT(_m.unlock_shared();)
}

}  // namespace yaclib::detail

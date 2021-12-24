#include <yaclib/fault/detail/mutex/shared_timed_mutex.hpp>

namespace yaclib::detail {

void SharedTimedMutex::lock() {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  YACLIB_INJECT_FAULT(_m.lock();)

  _exclusive_owner = me;
}

bool SharedTimedMutex::try_lock() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  YACLIB_INJECT_FAULT(auto res = _m.try_lock();)

  if (res) {
    _exclusive_owner = yaclib_std::this_thread::get_id();
  }
  return res;
}

void SharedTimedMutex::unlock() noexcept {
  assert(_exclusive_owner != yaclib::detail::kInvalidThreadId);
  assert(_exclusive_owner == yaclib_std::this_thread::get_id());

  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  YACLIB_INJECT_FAULT(_m.unlock();)
}

void SharedTimedMutex::lock_shared() {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  YACLIB_INJECT_FAULT(_m.lock_shared();)

  {
    std::unique_lock lock(_helper_m);
    _shared_owners.insert(me);
  }
}

bool SharedTimedMutex::try_lock_shared() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  YACLIB_INJECT_FAULT(auto res = _m.try_lock_shared();)

  if (res) {
    {
      std::unique_lock lock(_helper_m);
      _shared_owners.insert(me);
    }
  }
  return res;
}

void SharedTimedMutex::unlock_shared() noexcept {
  auto me = yaclib_std::this_thread::get_id();
  {
    std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) != _shared_owners.end());
  }

  {
    std::unique_lock lock(_helper_m);
    _shared_owners.erase(me);
  }
  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  YACLIB_INJECT_FAULT(_m.unlock_shared();)
}

}  // namespace yaclib::detail

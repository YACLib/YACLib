#include <yaclib/fault_injection/mutex/shared_mutex.hpp>

namespace yaclib::std::detail {

void SharedTimedMutex::lock() {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  _m.lock();
  _exclusive_owner = me;
  yaclib::detail::InjectFault();
}

bool SharedTimedMutex::try_lock() noexcept {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  auto res = _m.try_lock();
  if (res) {
    _exclusive_owner = yaclib::std::this_thread::get_id();
  }
  yaclib::detail::InjectFault();
  return res;
}

void SharedTimedMutex::unlock() noexcept {
  assert(_exclusive_owner != yaclib::std::detail::kInvalidThreadId);
  assert(_exclusive_owner == yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.unlock();
  _exclusive_owner = yaclib::std::detail::kInvalidThreadId;
  yaclib::detail::InjectFault();
}

template <class _Clock, class _Duration>
bool SharedTimedMutex::try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  auto res = _m.try_lock_until(duration);
  if (res) {
    _exclusive_owner = yaclib::std::this_thread::get_id();
  }
  yaclib::detail::InjectFault();
  return res;
}

void SharedTimedMutex::lock_shared() {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  _m.lock_shared();
  {
    ::std::unique_lock lock(_helper_m);
    _shared_owners.insert(me);
  }
  yaclib::detail::InjectFault();
}

bool SharedTimedMutex::try_lock_shared() noexcept {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  auto res = _m.try_lock_shared();
  if (res) {
    {
      ::std::unique_lock lock(_helper_m);
      _shared_owners.insert(me);
    }
  }
  yaclib::detail::InjectFault();
  return res;
}

void SharedTimedMutex::unlock_shared() noexcept {
  auto me = yaclib::std::this_thread::get_id();
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) != _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  _m.unlock_shared();
  {
    ::std::unique_lock lock(_helper_m);
    _shared_owners.erase(me);
  }
  _exclusive_owner = yaclib::std::detail::kInvalidThreadId;
  yaclib::detail::InjectFault();
}

template <class _Clock, class _Duration>
bool SharedTimedMutex::try_lock_shared_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }
  yaclib::detail::InjectFault();
  auto res = _m.try_lock_shared_until(duration);
  if (res) {
    {
      ::std::unique_lock lock(_helper_m);
      _shared_owners.insert(me);
    }
  }
  yaclib::detail::InjectFault();
  return res;
}

}  // namespace yaclib::std::detail

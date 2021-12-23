#include <yaclib/fault/detail/mutex/shared_timed_mutex.hpp>

namespace yaclib::detail {

void SharedTimedMutex::lock() {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  yaclib::detail::InjectFault();
  _m.lock();
  yaclib::detail::InjectFault();

  _exclusive_owner = me;
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
  yaclib::detail::InjectFault();

  if (res) {
    _exclusive_owner = yaclib::std::this_thread::get_id();
  }
  return res;
}

void SharedTimedMutex::unlock() noexcept {
  assert(_exclusive_owner != yaclib::detail::kInvalidThreadId);
  assert(_exclusive_owner == yaclib::std::this_thread::get_id());

  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  yaclib::detail::InjectFault();
  _m.unlock();
  yaclib::detail::InjectFault();
}

template <typename _Clock, typename _Duration>
bool SharedTimedMutex::try_lock_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  yaclib::detail::InjectFault();
  auto res = _m.try_lock_until(duration);
  yaclib::detail::InjectFault();

  if (res) {
    _exclusive_owner = yaclib::std::this_thread::get_id();
  }
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
  yaclib::detail::InjectFault();

  {
    ::std::unique_lock lock(_helper_m);
    _shared_owners.insert(me);
  }
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
  yaclib::detail::InjectFault();

  if (res) {
    {
      ::std::unique_lock lock(_helper_m);
      _shared_owners.insert(me);
    }
  }
  return res;
}

void SharedTimedMutex::unlock_shared() noexcept {
  auto me = yaclib::std::this_thread::get_id();
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) != _shared_owners.end());
  }

  {
    ::std::unique_lock lock(_helper_m);
    _shared_owners.erase(me);
  }
  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  yaclib::detail::InjectFault();
  _m.unlock_shared();
  yaclib::detail::InjectFault();
}

template <typename _Clock, typename _Duration>
bool SharedTimedMutex::try_lock_shared_until(const ::std::chrono::time_point<_Clock, _Duration>& duration) {
  auto me = yaclib::std::this_thread::get_id();
  assert(_exclusive_owner != me);
  {
    ::std::shared_lock lock(_helper_m);
    assert(_shared_owners.find(me) == _shared_owners.end());
  }

  yaclib::detail::InjectFault();
  auto res = _m.try_lock_shared_until(duration);
  yaclib::detail::InjectFault();

  if (res) {
    {
      ::std::unique_lock lock(_helper_m);
      _shared_owners.insert(me);
    }
  }
  return res;
}

}  // namespace yaclib::detail

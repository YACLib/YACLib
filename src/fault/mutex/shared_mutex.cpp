#include <yaclib/fault/detail/mutex/shared_mutex.hpp>

// TODO(myannyax) avoid copypaste from shared_timed_mutex
namespace yaclib::detail {

void SharedMutex::lock() {
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

bool SharedMutex::try_lock() noexcept {
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

void SharedMutex::unlock() noexcept {
  assert(_exclusive_owner != yaclib::detail::kInvalidThreadId);
  assert(_exclusive_owner == yaclib::std::this_thread::get_id());

  _exclusive_owner = yaclib::detail::kInvalidThreadId;

  yaclib::detail::InjectFault();
  _m.unlock();
  yaclib::detail::InjectFault();
}

void SharedMutex::lock_shared() {
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

bool SharedMutex::try_lock_shared() noexcept {
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

void SharedMutex::unlock_shared() noexcept {
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

}  // namespace yaclib::detail

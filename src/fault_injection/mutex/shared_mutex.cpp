#include <yaclib/fault_injection/mutex/shared_mutex.hpp>

// TODO(myannyax) avoid copypaste from shared_timed_mutex
namespace yaclib::std::detail {

void SharedMutex::lock() {
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

bool SharedMutex::try_lock() noexcept {
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

void SharedMutex::unlock() noexcept {
  assert(_exclusive_owner != yaclib::std::detail::kInvalidThreadId);
  assert(_exclusive_owner == yaclib::std::this_thread::get_id());
  yaclib::detail::InjectFault();
  _m.unlock();
  _exclusive_owner = yaclib::std::detail::kInvalidThreadId;
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
  {
    ::std::unique_lock lock(_helper_m);
    _shared_owners.insert(me);
  }
  yaclib::detail::InjectFault();
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
  if (res) {
    {
      ::std::unique_lock lock(_helper_m);
      _shared_owners.insert(me);
    }
  }
  yaclib::detail::InjectFault();
  return res;
}

void SharedMutex::unlock_shared() noexcept {
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

}  // namespace yaclib::std::detail

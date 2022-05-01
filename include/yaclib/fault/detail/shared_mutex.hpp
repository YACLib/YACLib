#pragma once

#include <yaclib/fault/detail/mutex.hpp>
#include <yaclib/fault/inject.hpp>

namespace yaclib::detail {

template <typename Impl>
class SharedMutex : public Mutex<Impl> {
  using Base = Mutex<Impl>;

 public:
  using Base::Base;

  void lock_shared() {
    YACLIB_INJECT_FAULT(Impl::lock_shared());
  }

  bool try_lock_shared() {
    YACLIB_INJECT_FAULT(auto r = Impl::try_lock_shared());
    return r;
  }

  void unlock_shared() {
    YACLIB_INJECT_FAULT(Impl::unlock_shared());
  }
};

}  // namespace yaclib::detail

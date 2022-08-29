#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/detail/fiber/atomic_wait.hpp>
#include <yaclib/fault/inject.hpp>

namespace yaclib::detail::fiber {

class AtomicFlag : public AtomicWait<bool> {
  using Base = AtomicWait<bool>;

 public:
  using Base::Base;

  void clear(std::memory_order) volatile noexcept {
    _value = false;
  }
  void clear(std::memory_order) noexcept {
    _value = false;
  }

  bool test_and_set(std::memory_order) volatile noexcept {
    auto val = _value;
    _value = true;
    return val;
  }
  bool test_and_set(std::memory_order) noexcept {
    auto val = _value;
    _value = true;
    return val;
  }

#if YACLIB_FUTEX != 0
  bool test(std::memory_order) const volatile noexcept {
    return _value;
  }
  bool test(std::memory_order) const noexcept {
    return _value;
  }
#endif
 private:
  using Base::_value;
};

}  // namespace yaclib::detail::fiber

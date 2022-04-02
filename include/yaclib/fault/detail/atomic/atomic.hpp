#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/detail/atomic/atomic_base.hpp>

namespace yaclib::detail {

template <typename T>
struct Atomic : public AtomicBase<T, std::atomic<T>> {
  Atomic() noexcept = default;

  constexpr Atomic(T desired) noexcept : AtomicBase<T, std::atomic<T>>(desired) {
  }

  T operator=(T desired) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.operator=(desired));
    return result;
  }

  T operator=(T desired) noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.operator=(desired));
    return result;
  }

#ifdef YACLIB_ATOMIC_EVENT
  void wait(T value, std::memory_order mo = std::memory_order_seq_cst) const volatile noexcept {
    YACLIB_INJECT_FAULT(this->_impl.wait(value, mo));
  }
  void wait(T value, std::memory_order mo = std::memory_order_seq_cst) const noexcept {
    YACLIB_INJECT_FAULT(this->_impl.wait(value, mo));
  }
  void notify_one() volatile noexcept {
    YACLIB_INJECT_FAULT(this->_impl.notify_one());
  }
  void notify_one() noexcept {
    YACLIB_INJECT_FAULT(this->_impl.notify_one());
  }
  void notify_all() volatile noexcept {
    YACLIB_INJECT_FAULT(this->_impl.notify_all());
  }
  void notify_all() noexcept {
    YACLIB_INJECT_FAULT(this->_impl.notify_all());
  }
#endif
};

}  // namespace yaclib::detail

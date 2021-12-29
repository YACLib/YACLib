#pragma once
#include <yaclib/fault/detail/atomic/atomic_base.hpp>

#include <atomic>

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
};

}  // namespace yaclib::detail

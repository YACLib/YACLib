#pragma once
#include <yaclib/fault/detail/atomic/atomic_base.hpp>

#include <atomic>

namespace yaclib::detail {

template <class T>
struct Atomic : public AtomicBase<T, ::std::atomic<T>> {
 protected:
  ::std::atomic<T> GetDelegate() override {
    return _delegate;
  }

  Atomic() noexcept = default;

  constexpr Atomic(T desired) noexcept : _delegate(desired) {
  }

  T operator=(T desired) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = _delegate.operator=(desired);
    yaclib::detail::InjectFault();
    return result;
  }

  T operator=(T desired) noexcept {
    yaclib::detail::InjectFault();
    auto result = _delegate.operator=(desired);
    yaclib::detail::InjectFault();
    return result;
  }

 private:
  ::std::atomic<T> _delegate;
};

}  // namespace yaclib::detail

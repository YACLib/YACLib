#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/detail/atomic_wait.hpp>
#include <yaclib/fault/inject.hpp>

namespace yaclib::detail {

template <typename Impl>
class AtomicFlag : public AtomicWait<Impl, bool> {
  using Base = AtomicWait<Impl, bool>;

 public:
  using Base::Base;

  void clear(std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(Impl::clear(order));
  }
  void clear(std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(Impl::clear(order));
  }

  bool test_and_set(std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::test_and_set(order));
    return r;
  }
  bool test_and_set(std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::test_and_set(order));
    return r;
  }

#if YACLIB_FUTEX != 0
  bool test(std::memory_order order = std::memory_order::seq_cst) const volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::test(order));
    return r;
  }
  bool test(std::memory_order order = std::memory_order::seq_cst) const noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::test(order));
    return r;
  }
#endif
};

}  // namespace yaclib::detail

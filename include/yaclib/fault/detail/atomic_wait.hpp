#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/inject.hpp>

namespace yaclib::detail {

template <typename Impl, typename T>
class AtomicWait : protected Impl {
 public:
  using Impl::Impl;

#if YACLIB_FUTEX != 0
  void wait(T old, std::memory_order order = std::memory_order_seq_cst) const noexcept {
    YACLIB_INJECT_FAULT(Impl::wait(old, order));
  }
  void wait(T old, std::memory_order order = std::memory_order_seq_cst) const volatile noexcept {
    YACLIB_INJECT_FAULT(Impl::wait(old, order));
  }

  void notify_one() noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_one());
  }
  void notify_one() volatile noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_one());
  }

  void notify_all() noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_all());
  }
  void notify_all() volatile noexcept {
    YACLIB_INJECT_FAULT(Impl::notify_all());
  }
#endif
};

}  // namespace yaclib::detail

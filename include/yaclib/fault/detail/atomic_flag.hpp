#pragma once

#include <yaclib/config.hpp>

namespace yaclib::detail {

template <typename Impl>
class AtomicFlag {
 public:
#ifdef YACLIB_ATOMIC_EVENT
  void wait(T value, std::memory_order order = std::memory_order_seq_cst) const volatile noexcept {
    YACLIB_INJECT_FAULT(_impl.wait(value, mo));
  }
  void wait(T value, std::memory_order order = std::memory_order_seq_cst) const noexcept {
    YACLIB_INJECT_FAULT(_impl.wait(value, mo));
  }
  void notify_one() volatile noexcept {
    YACLIB_INJECT_FAULT(_impl.notify_one());
  }
  void notify_one() noexcept {
    YACLIB_INJECT_FAULT(_impl.notify_one());
  }
  void notify_all() volatile noexcept {
    YACLIB_INJECT_FAULT(_impl.notify_all());
  }
  void notify_all() noexcept {
    YACLIB_INJECT_FAULT(_impl.notify_all());
  }
#endif
 private:
  Impl _impl;
};

}  // namespace yaclib::detail

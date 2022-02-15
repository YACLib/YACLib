#pragma once

#include <yaclib/fault/detail/inject_fault.hpp>

#include <atomic>

namespace yaclib::detail {

class AtomicFlag {
 public:
  bool test_and_set(std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.test_and_set(order));
    return result;
  }

  bool test_and_set(std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.test_and_set(order));
    return result;
  }

  void clear(std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(_impl.clear(order));
  }

  void clear(std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(_impl.clear(order));
  }

  AtomicFlag() noexcept = default;

  AtomicFlag(const AtomicFlag&) = delete;
  AtomicFlag& operator=(const AtomicFlag&) = delete;
  AtomicFlag& operator=(const AtomicFlag&) volatile = delete;

 private:
  std::atomic_flag _impl;
};

}  // namespace yaclib::detail

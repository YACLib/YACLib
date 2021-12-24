#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <atomic>

namespace yaclib::detail {

struct AtomicFlag {
  bool test_and_set(::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = _delegate.test_and_set(order);
    yaclib::detail::InjectFault();
    return result;
  }

  bool test_and_set(::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = _delegate.test_and_set(order);
    yaclib::detail::InjectFault();
    return result;
  }

  void clear(::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    _delegate.clear(order);
    yaclib::detail::InjectFault();
  }

  void clear(::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    _delegate.clear(order);
    yaclib::detail::InjectFault();
  }

  AtomicFlag() noexcept = default;

  constexpr AtomicFlag(bool value) noexcept : _delegate(value) {
  }  // EXTENSION

  AtomicFlag(const AtomicFlag&) = delete;
  AtomicFlag& operator=(const AtomicFlag&) = delete;
  AtomicFlag& operator=(const AtomicFlag&) volatile = delete;

 private:
  ::std::atomic_flag _delegate;
};

}  // namespace yaclib::detail

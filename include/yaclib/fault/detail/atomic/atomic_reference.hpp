#pragma once

#include <yaclib/fault/detail/atomic/atomic.hpp>

namespace yaclib::detail {
template <class T>
struct Atomic<T*> {
  Atomic() noexcept = default;

  constexpr Atomic(T* desired) noexcept : _delegate(desired) {
  }

  T* operator=(T* desired) volatile noexcept {
    auto result = _delegate.operator=(desired);
    return result;
  }

  T* operator=(T* desired) noexcept {
    auto result = _delegate.operator=(desired);
    return result;
  }

  T* fetch_add(ptrdiff_t arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    auto result = _delegate.fetch_add(arg, order);
    return result;
  }

  T* fetch_add(ptrdiff_t arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    auto result = _delegate.fetch_add(arg, order);
    return result;
  }

  T* fetch_sub(ptrdiff_t arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    auto result = _delegate.fetch_sub(arg, order);
    return result;
  }

  T* fetch_sub(ptrdiff_t arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    auto result = _delegate.fetch_sub(arg, order);
    return result;
  }

  T* operator++(int) volatile noexcept {
    return fetch_add(1);
  }

  T* operator++(int) noexcept {
    return fetch_add(1);
  }

  T* operator--(int) volatile noexcept {
    return fetch_sub(1);
  }

  T* operator--(int) noexcept {
    return fetch_sub(1);
  }

  T* operator++() volatile noexcept {
    return fetch_add(1) + 1;
  }

  T* operator++() noexcept {
    return fetch_add(1) + 1;
  }

  T* operator--() volatile noexcept {
    return fetch_sub(1) - 1;
  }

  T* operator--() noexcept {
    return fetch_sub(1) - 1;
  }

  T* operator+=(ptrdiff_t arg) volatile noexcept {
    return fetch_add(arg) + arg;
  }

  T* operator+=(ptrdiff_t arg) noexcept {
    return fetch_add(arg) + arg;
  }

  T* operator-=(ptrdiff_t arg) volatile noexcept {
    return fetch_sub(arg) - arg;
  }

  T* operator-=(ptrdiff_t arg) noexcept {
    return fetch_sub(arg) - arg;
  }

 private:
  ::std::atomic<T*> _delegate;
};
}  // namespace yaclib::detail

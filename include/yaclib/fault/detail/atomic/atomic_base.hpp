#pragma once

#include <yaclib/fault/detail/inject_fault.hpp>

#include <atomic>
#include <type_traits>

// TODO(myannyax) separate header for fibers?
namespace yaclib::detail {

// TODO(myannyax) is_always_lock_free
template <typename T, typename Atomic, bool = std::is_integral<T>::value && !std::is_same<T, bool>::value>
struct AtomicBase {
  [[nodiscard]] bool is_lock_free() const volatile noexcept {
    return _impl.is_lock_free();
  }

  [[nodiscard]] bool is_lock_free() const noexcept {
    return _impl.is_lock_free();
  }

  void store(T desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(_impl.store(desired, order));
  }

  void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(_impl.store(desired, order));
  }

  T load(std::memory_order order = std::memory_order_seq_cst) const volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.load(order));
    return result;
  }

  T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.load(order));
    return result;
  }

  operator T() const volatile noexcept {
    return load();
  }

  operator T() const noexcept {
    return load();
  }

  T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.exchange(desired, order));
    return result;
  }

  T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.exchange(desired, order));
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order success,
                             std::memory_order failure) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_weak(expected, desired, success, failure));
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_weak(expected, desired, success, failure));
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired, std::memory_order success,
                               std::memory_order failure) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_strong(expected, desired, success, failure));
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_strong(expected, desired, success, failure));
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired,
                             std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_weak(expected, desired, order));
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_weak(expected, desired, order));
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired,
                               std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_strong(expected, desired, order));
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = _impl.compare_exchange_strong(expected, desired, order));
    return result;
  }

  AtomicBase() noexcept = default;

  constexpr AtomicBase(T desired) noexcept : _impl(desired) {
  }

  AtomicBase(const AtomicBase&) = delete;
  AtomicBase& operator=(const AtomicBase&) = delete;
  AtomicBase& operator=(const AtomicBase&) volatile = delete;

 protected:
  Atomic _impl;
};

template <typename T, typename Atomic>
struct AtomicBase<T, Atomic, true> : public AtomicBase<T, Atomic, false> {
  AtomicBase() noexcept = default;

  constexpr AtomicBase(T desired) noexcept : AtomicBase<T, Atomic, false>(desired) {
  }

  T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_add(arg, order));
    return result;
  }

  T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_add(arg, order));
    return result;
  }

  T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_sub(arg, order));
    return result;
  }

  T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_sub(arg, order));
    return result;
  }

  T fetch_and(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_and(arg, order));
    return result;
  }

  T fetch_and(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_and(arg, order));
    return result;
  }

  T fetch_or(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_or(arg, order));
    return result;
  }

  T fetch_or(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_or(arg, order));
    return result;
  }

  T fetch_xor(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_xor(arg, order));
    return result;
  }

  T fetch_xor(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto result = this->_impl.fetch_xor(arg, order));
    return result;
  }

  T operator++(int) volatile noexcept {
    return fetch_add(T(1));
  }

  T operator++(int) noexcept {
    return fetch_add(T(1));
  }

  T operator--(int) volatile noexcept {
    return fetch_sub(T(1));
  }

  T operator--(int) noexcept {
    return fetch_sub(T(1));
  }

  T operator++() volatile noexcept {
    return fetch_add(T(1)) + T(1);
  }

  T operator++() noexcept {
    return fetch_add(T(1)) + T(1);
  }

  T operator--() volatile noexcept {
    return fetch_sub(T(1)) - T(1);
  }

  T operator--() noexcept {
    return fetch_sub(T(1)) - T(1);
  }

  T operator+=(T arg) volatile noexcept {
    return fetch_add(arg) + arg;
  }

  T operator+=(T arg) noexcept {
    return fetch_add(arg) + arg;
  }

  T operator-=(T arg) volatile noexcept {
    return fetch_sub(arg) - arg;
  }

  T operator-=(T arg) noexcept {
    return fetch_sub(arg) - arg;
  }

  T operator&=(T arg) volatile noexcept {
    return fetch_and(arg) & arg;
  }

  T operator&=(T arg) noexcept {
    return fetch_and(arg) & arg;
  }

  T operator|=(T arg) volatile noexcept {
    return fetch_or(arg) | arg;
  }

  T operator|=(T arg) noexcept {
    return fetch_or(arg) | arg;
  }

  T operator^=(T arg) volatile noexcept {
    return fetch_xor(arg) ^ arg;
  }

  T operator^=(T arg) noexcept {
    return fetch_xor(arg) ^ arg;
  }
};

}  // namespace yaclib::detail

#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <atomic>
#include <type_traits>

// TODO(myannyax) separate header for fibers?
namespace yaclib::detail {

// TODO(myannyax) is_always_lock_free
template <class T, class Atomic, bool = ::std::is_integral<T>::desired && !::std::is_same<T, bool>::desired>
struct AtomicBase {
  [[nodiscard]] bool is_lock_free() const volatile noexcept {
    return GetDelegate().is_lock_free();
  }

  [[nodiscard]] bool is_lock_free() const noexcept {
    return GetDelegate().is_lock_free();
  }

  void store(T desired, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    GetDelegate().store(desired, order);
    yaclib::detail::InjectFault();
  }

  void store(T desired, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    GetDelegate().store(desired, order);
    yaclib::detail::InjectFault();
  }

  T load(::std::memory_order order = ::std::memory_order_seq_cst) const volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().load(order);
    yaclib::detail::InjectFault();
    return result;
  }

  T load(::std::memory_order order = ::std::memory_order_seq_cst) const noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().load(order);
    yaclib::detail::InjectFault();
    return result;
  }

  operator T() const volatile noexcept {
    return load();
  }

  operator T() const noexcept {
    return load();
  }

  T exchange(T desired, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().exchange(desired, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T exchange(T desired, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().exchange(desired, order);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired, ::std::memory_order success,
                             ::std::memory_order failure) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_weak(expected, desired, success, failure);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired, ::std::memory_order success,
                             ::std::memory_order failure) noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_weak(expected, desired, success, failure);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired, ::std::memory_order success,
                               ::std::memory_order failure) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_strong(expected, desired, success, failure);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired, ::std::memory_order success,
                               ::std::memory_order failure) noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_strong(expected, desired, success, failure);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired,
                             ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_weak(expected, desired, order);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_weak(T& expected, T desired, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_weak(expected, desired, order);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired,
                               ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_strong(expected, desired, order);
    yaclib::detail::InjectFault();
    return result;
  }

  bool compare_exchange_strong(T& expected, T desired,
                               ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = GetDelegate().compare_exchange_strong(expected, desired, order);
    yaclib::detail::InjectFault();
    return result;
  }

  AtomicBase() noexcept = default;

  // TODO(myannyax) check it's present in overrides
  /*constexpr AtomicBase(T desired) noexcept {
  }*/

  AtomicBase(const AtomicBase&) = delete;
  AtomicBase& operator=(const AtomicBase&) = delete;
  AtomicBase& operator=(const AtomicBase&) volatile = delete;

 protected:
  virtual Atomic GetDelegate() = 0;
};

template <class T, class Atomic>
struct AtomicBase<T, Atomic, true> : public AtomicBase<T, Atomic, false> {
  AtomicBase() noexcept = default;

  // TODO(myannyax) check it's present in overrides
  /*constexpr AtomicBase(T __d) noexcept : __base(__d) {
  }*/

  T fetch_add(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_add(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_add(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_add(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_sub(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_sub(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_sub(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_sub(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_and(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_and(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_and(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_and(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_or(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_or(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_or(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_or(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_xor(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) volatile noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_xor(arg, order);
    yaclib::detail::InjectFault();
    return result;
  }

  T fetch_xor(T arg, ::std::memory_order order = ::std::memory_order_seq_cst) noexcept {
    yaclib::detail::InjectFault();
    auto result = this->GetDelegate().fetch_xor(arg, order);
    yaclib::detail::InjectFault();
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

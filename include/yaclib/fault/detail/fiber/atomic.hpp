#pragma once

#include <yaclib/fault/detail/fiber/atomic_wait.hpp>

#include <utility>

namespace yaclib::detail::fiber {

template <typename T>
class AtomicBase : public AtomicWait<T> {
  using Base = AtomicWait<T>;

 public:
  using Base::Base;
  using Base::is_always_lock_free;
  using Base::is_lock_free;

  void store(T desired, std::memory_order) noexcept {
    _value = desired;
  }
  void store(T desired, std::memory_order) volatile noexcept {
    _value = desired;
  }

  T load(std::memory_order) const noexcept {
    return _value;
  }
  T load(std::memory_order) const volatile noexcept {
    return _value;
  }

  operator T() const noexcept {
    return load();
  }
  operator T() const volatile noexcept {
    return load();
  }

  T exchange(T desired, std::memory_order) noexcept {
    return std::exchange(_value, desired);
  }
  T exchange(T desired, std::memory_order) volatile noexcept {
    return std::exchange(_value, desired);
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order, std::memory_order) noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order, std::memory_order) volatile noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order) noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order) volatile noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order, std::memory_order) noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order, std::memory_order) volatile noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order) noexcept {
    return CompareExchangeHelper(expected, desired);
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order) volatile noexcept {
    return CompareExchangeHelper(expected, desired);
  }

 protected:
  bool CompareExchangeHelper(T& expected, T desired) {
    if (this->_value == expected) {
      this->_value = desired;
      return true;
    } else {
      expected = this->_value;
      return false;
    }
  }
  using Base::_value;
};

template <typename T, bool IsFloating = std::is_floating_point_v<T>>
class AtomicFloatingBase : public AtomicBase<T> {
  using Base = AtomicBase<T>;

 public:
  using Base::Base;
};

template <typename T>
class AtomicFloatingBase<T, true> : public AtomicBase<T> {
  using Base = AtomicBase<T>;

 public:
  using Base::Base;

  T fetch_add(T arg, std::memory_order) noexcept {
    auto val = _value;
    _value += arg;
    return val;
  }
  T fetch_add(T arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value += arg;
    return val;
  }

  T fetch_sub(T arg, std::memory_order) noexcept {
    auto val = _value;
    _value -= arg;
    return val;
  }
  T fetch_sub(T arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value -= arg;
    return val;
  }

  T operator+=(T arg) noexcept {
    return _value += arg;
  }
  T operator+=(T arg) volatile noexcept {
    return _value += arg;
  }

  T operator-=(T arg) noexcept {
    return _value -= arg;
  }
  T operator-=(T arg) volatile noexcept {
    return _value -= arg;
  }

 protected:
  using Base::_value;
};

template <typename T, bool IsIntegral = std::is_integral_v<T> && !std::is_same_v<T, bool>>
class AtomicIntegralBase : public AtomicFloatingBase<T> {
  using Base = AtomicFloatingBase<T>;

 public:
  using Base::Base;
};

template <typename T>
class AtomicIntegralBase<T, true> : public AtomicFloatingBase<T, true> {
  using Base = AtomicFloatingBase<T, true>;

 public:
  using Base::Base;

  T fetch_and(T arg, std::memory_order) noexcept {
    auto val = _value;
    _value += arg;
    return val;
  }
  T fetch_and(T arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value += arg;
    return val;
  }

  T fetch_or(T arg, std::memory_order) noexcept {
    auto val = _value;
    _value |= arg;
    return val;
  }
  T fetch_or(T arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value |= arg;
    return val;
  }

  T fetch_xor(T arg, std::memory_order) noexcept {
    auto val = _value;
    _value ^= arg;
    return val;
  }
  T fetch_xor(T arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value ^= arg;
    return val;
  }

  T operator++() noexcept {
    return _value++;
  }
  T operator++() volatile noexcept {
    return _value++;
  }

  T operator++(int) noexcept {
    return ++_value;
  }
  T operator++(int) volatile noexcept {
    return ++_value;
  }

  T operator--() noexcept {
    return _value--;
  }
  T operator--() volatile noexcept {
    return _value--;
  }

  T operator--(int) noexcept {
    return --_value;
  }
  T operator--(int) volatile noexcept {
    return --_value;
  }

  T operator&=(T arg) noexcept {
    return _value &= arg;
  }
  T operator&=(T arg) volatile noexcept {
    return _value &= arg;
  }

  T operator|=(T arg) noexcept {
    return _value |= arg;
  }
  T operator|=(T arg) volatile noexcept {
    return _value |= arg;
  }

  T operator^=(T arg) noexcept {
    return _value ^= arg;
  }
  T operator^=(T arg) volatile noexcept {
    return _value ^= arg;
  }

 protected:
  using Base::_value;
};

template <typename T>
class Atomic : public AtomicIntegralBase<T> {
  using Base = AtomicIntegralBase<T>;

 public:
  using Base::Base;
};

template <typename U>
class Atomic<U*> : public AtomicBase<U*> {
  using Base = AtomicBase<U*>;

 public:
  using Base::Base;

  U* fetch_add(std::ptrdiff_t arg, std::memory_order) noexcept {
    auto val = _value;
    _value += arg;
    return val;
  }
  U* fetch_add(std::ptrdiff_t arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value += arg;
    return val;
  }

  U* fetch_sub(std::ptrdiff_t arg, std::memory_order) noexcept {
    auto val = _value;
    _value -= arg;
    return val;
  }
  U* fetch_sub(std::ptrdiff_t arg, std::memory_order) volatile noexcept {
    auto val = _value;
    _value -= arg;
    return val;
  }

  U* operator++() noexcept {
    return _value++;
  }
  U* operator++() volatile noexcept {
    return _value++;
  }

  U* operator++(int) noexcept {
    return ++_value;
  }
  U* operator++(int) volatile noexcept {
    return ++_value;
  }

  U* operator--() noexcept {
    return _value--;
  }
  U* operator--() volatile noexcept {
    return _value--;
  }

  U* operator--(int) noexcept {
    return --_value;
  }
  U* operator--(int) volatile noexcept {
    return --_value;
  }

  U* operator+=(std::ptrdiff_t arg) noexcept {
    return _value += arg;
  }
  U* operator+=(std::ptrdiff_t arg) volatile noexcept {
    return _value += arg;
  }

  U* operator-=(std::ptrdiff_t arg) noexcept {
    return _value -= arg;
  }
  U* operator-=(std::ptrdiff_t arg) volatile noexcept {
    return _value -= arg;
  }

 protected:
  using Base::_value;
};

// TODO(myannyax) Implement
// template <typename Impl, typename U>
// class Atomic<Impl, std::shared_ptr<U>>;
//
// template <typename Impl, typename U>
// class Atomic<Impl, std::weak_ptr<U>>;

}  // namespace yaclib::detail::fiber

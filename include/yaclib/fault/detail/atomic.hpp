#pragma once

#include <yaclib/fault/detail/atomic_wait.hpp>
#include <yaclib/fault/inject.hpp>

#include <type_traits>

namespace yaclib::detail {

bool ShouldFailAtomicWeak();

void SetAtomicWeakFailFrequency(std::uint32_t k);

template <typename Impl, typename T>
class AtomicBase : public AtomicWait<Impl, T> {
  using Base = AtomicWait<Impl, T>;

 public:
  using Base::Base;
  using Base::is_always_lock_free;
  using Base::is_lock_free;

  T operator=(T desired) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator=(desired));
    return r;
  }
  T operator=(T desired) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator=(desired));
    return r;
  }

  void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(Impl::store(desired, order));
  }
  void store(T desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(Impl::store(desired, order));
  }

  T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::load(order));
    return r;
  }
  T load(std::memory_order order = std::memory_order_seq_cst) const volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::load(order));
    return r;
  }

  operator T() const noexcept {
    return load();
  }
  operator T() const volatile noexcept {
    // YACLIB_INJECT_FAULT(auto r = Impl::operator T()());
    return load();
  }

  T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::exchange(desired, order));
    return r;
  }
  T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::exchange(desired, order));
    return r;
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept {
    if (ShouldFailAtomicWeak()) {
      expected = load(failure);
      return false;
    }
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_weak(expected, desired, success, failure));
    return r;
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order success,
                             std::memory_order failure) volatile noexcept {
    if (ShouldFailAtomicWeak()) {
      expected = load(failure);
      return false;
    }
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_weak(expected, desired, success, failure));
    return r;
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    if (ShouldFailAtomicWeak()) {
      expected = load(order);
      return false;
    }
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_weak(expected, desired, order));
    return r;
  }
  bool compare_exchange_weak(T& expected, T desired,
                             std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    if (ShouldFailAtomicWeak()) {
      expected = load(order);
      return false;
    }
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_weak(expected, desired, order));
    return r;
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_strong(expected, desired, success, failure));
    return r;
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order success,
                               std::memory_order failure) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_strong(expected, desired, success, failure));
    return r;
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_strong(expected, desired, order));
    return r;
  }
  bool compare_exchange_strong(T& expected, T desired,
                               std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::compare_exchange_strong(expected, desired, order));
    return r;
  }
};

template <typename Impl, typename T, bool IsFloating = std::is_floating_point_v<T>>
class AtomicFloatingBase : public AtomicBase<Impl, T> {
  using Base = AtomicBase<Impl, T>;

 public:
  using Base::Base;
};

template <typename Impl, typename T>
class AtomicFloatingBase<Impl, T, true> : public AtomicBase<Impl, T> {
  using Base = AtomicBase<Impl, T>;

 public:
  using Base::Base;

  T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_add(arg, order));
    return r;
  }
  T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_add(arg, order));
    return r;
  }

  T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_sub(arg, order));
    return r;
  }
  T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_sub(arg, order));
    return r;
  }

  T operator+=(T arg) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator+=(arg));
    return r;
  }
  T operator+=(T arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator+=(arg));
    return r;
  }

  T operator-=(T arg) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator-=(arg));
    return r;
  }
  T operator-=(T arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator-=(arg));
    return r;
  }
};

template <typename Impl, typename T, bool IsIntegral = std::is_integral_v<T> && !std::is_same_v<T, bool>>
class AtomicIntegralBase : public AtomicFloatingBase<Impl, T> {
  using Base = AtomicFloatingBase<Impl, T>;

 public:
  using Base::Base;
};

template <typename Impl, typename T>
class AtomicIntegralBase<Impl, T, true> : public AtomicFloatingBase<Impl, T, true> {
  using Base = AtomicFloatingBase<Impl, T, true>;

 public:
  using Base::Base;

  T fetch_and(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_and(arg, order));
    return r;
  }
  T fetch_and(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_and(arg, order));
    return r;
  }

  T fetch_or(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_or(arg, order));
    return r;
  }
  T fetch_or(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_or(arg, order));
    return r;
  }

  T fetch_xor(T arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_xor(arg, order));
    return r;
  }
  T fetch_xor(T arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::fetch_xor(arg, order));
    return r;
  }

  T operator++() noexcept {
    YACLIB_INJECT_FAULT(auto r = ++static_cast<Impl&>(*this));
    return r;
  }
  T operator++() volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = ++static_cast<Impl&>(*this));
    return r;
  }

  T operator++(int) noexcept {
    YACLIB_INJECT_FAULT(auto r = static_cast<Impl&>(*this)++);
    return r;
  }
  T operator++(int) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = static_cast<Impl&>(*this)++);
    return r;
  }

  T operator--() noexcept {
    YACLIB_INJECT_FAULT(auto r = --static_cast<Impl&>(*this));
    return r;
  }
  T operator--() volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = --static_cast<Impl&>(*this));
    return r;
  }

  T operator--(int) noexcept {
    YACLIB_INJECT_FAULT(auto r = static_cast<Impl&>(*this)--);
    return r;
  }
  T operator--(int) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = static_cast<Impl&>(*this)--);
    return r;
  }

  T operator&=(T arg) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator&=(arg));
    return r;
  }
  T operator&=(T arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator&=(arg));
    return r;
  }

  T operator|=(T arg) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator|=(arg));
    return r;
  }
  T operator|=(T arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator|=(arg));
    return r;
  }

  T operator^=(T arg) noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator^=(arg));
    return r;
  }
  T operator^=(T arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto r = Impl::operator^=(arg));
    return r;
  }
};

template <typename Impl, typename T>
class Atomic : public AtomicIntegralBase<Impl, T> {
  using Base = AtomicIntegralBase<Impl, T>;

 public:
  using Base::Base;
};

template <typename Impl, typename U>
class Atomic<Impl, U*> : public AtomicBase<Impl, U*> {
  using Base = AtomicBase<Impl, U*>;

 public:
  using Base::Base;

  U* fetch_add(std::ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::fetch_add(arg, order));
    return r;
  }
  U* fetch_add(std::ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::fetch_add(arg, order));
    return r;
  }

  U* fetch_sub(std::ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::fetch_sub(arg, order));
    return r;
  }
  U* fetch_sub(std::ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::fetch_sub(arg, order));
    return r;
  }

  U* operator++() noexcept {
    YACLIB_INJECT_FAULT(auto* r = ++static_cast<Impl&>(*this));
    return r;
  }
  U* operator++() volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = ++static_cast<Impl&>(*this));
    return r;
  }

  U* operator++(int) noexcept {
    YACLIB_INJECT_FAULT(auto* r = static_cast<Impl&>(*this)++);
    return r;
  }
  U* operator++(int) volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = static_cast<Impl&>(*this)++);
    return r;
  }

  U* operator--() noexcept {
    YACLIB_INJECT_FAULT(auto* r = --static_cast<Impl&>(*this));
    return r;
  }
  U* operator--() volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = --static_cast<Impl&>(*this));
    return r;
  }

  U* operator--(int) noexcept {
    YACLIB_INJECT_FAULT(auto* r = static_cast<Impl&>(*this)--);
    return r;
  }
  U* operator--(int) volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = static_cast<Impl&>(*this)--);
    return r;
  }

  U* operator+=(std::ptrdiff_t arg) noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::operator+=(arg));
    return r;
  }
  U* operator+=(std::ptrdiff_t arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::operator+=(arg));
    return r;
  }

  U* operator-=(std::ptrdiff_t arg) noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::operator-=(arg));
    return r;
  }
  U* operator-=(std::ptrdiff_t arg) volatile noexcept {
    YACLIB_INJECT_FAULT(auto* r = Impl::operator-=(arg));
    return r;
  }
};

// TODO(myannyax) Implement
// template <typename Impl, typename U>
// class Atomic<Impl, std::shared_ptr<U>>;
//
// template <typename Impl, typename U>
// class Atomic<Impl, std::weak_ptr<U>>;

}  // namespace yaclib::detail

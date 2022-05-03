#pragma once
#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <string_view>
#include <unordered_map>

namespace yaclib::detail::fiber {

static std::unordered_map<uint64_t, void*> defaults{};
static uint64_t nextFreeIndex{0};

template <typename Type>
class ThreadLocalPtrProxy {
 public:
  ThreadLocalPtrProxy() : _i(nextFreeIndex++) {
    defaults[_i] = nullptr;
  }

  ThreadLocalPtrProxy(Type* value) : _i(nextFreeIndex++) {
    defaults[_i] = value;
  }
  ThreadLocalPtrProxy(ThreadLocalPtrProxy&& other) : _i(other._i) {
  }
  ThreadLocalPtrProxy(const ThreadLocalPtrProxy& other) : _i(nextFreeIndex++) {
    defaults[_i] = other.Get();
  }

  ThreadLocalPtrProxy& operator=(Type* value) {
    auto* fiber = fault::Scheduler::Current();
    fiber->SetTls(_i, value);
    return *this;
  }
  ThreadLocalPtrProxy& operator=(ThreadLocalPtrProxy&& other) noexcept {
    _i = other._i;
    return *this;
  }
  ThreadLocalPtrProxy& operator=(const ThreadLocalPtrProxy& other) {
    defaults[_i] = other.Get();
    return *this;
  }

  template <typename U>
  ThreadLocalPtrProxy(ThreadLocalPtrProxy<U>&& other) noexcept : _i(other._i) {
  }
  template <typename U>
  ThreadLocalPtrProxy(const ThreadLocalPtrProxy<U>& other) noexcept : _i(nextFreeIndex++) {
    defaults[_i] = other.Get();
  }

  template <typename U>
  ThreadLocalPtrProxy& operator=(ThreadLocalPtrProxy<U>&& other) noexcept {
    _i = other._i;
    return *this;
  }
  template <typename U>
  ThreadLocalPtrProxy& operator=(const ThreadLocalPtrProxy<U>& other) noexcept {
    defaults[_i] = other.Get();
    return *this;
  }

  Type& operator*() const noexcept {
    auto* fiber = fault::Scheduler::Current();
    auto& val = *(static_cast<Type*>(fiber->GetTls(_i, defaults[_i])));
    return val;
  }

  Type* operator->() const noexcept {
    return Get();
  }

  Type* Get() const noexcept {
    auto* fiber = fault::Scheduler::Current();
    return static_cast<Type*>(fiber->GetTls(_i, defaults[_i]));
  }

  explicit operator bool() const noexcept {
    auto* fiber = fault::Scheduler::Current();
    auto* val = static_cast<Type*>(fiber->GetTls(_i, defaults[_i]));
    return val != nullptr;
  }

  Type& operator[](std::size_t i) const {
    auto* fiber = fault::Scheduler::Current();
    auto* val = static_cast<Type*>(fiber->GetTls(_i, defaults[_i]));
    val += i;
    return *val;
  }

 private:
  uint64_t _i;
};

template <typename T, typename U>
inline bool operator==(const ThreadLocalPtrProxy<T>& lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs.Get() == rhs.Get();
}

template <typename T, typename U>
inline bool operator!=(const ThreadLocalPtrProxy<T>& lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs.Get() != rhs.Get();
}

template <typename T, typename U>
inline bool operator==(const ThreadLocalPtrProxy<T>& lhs, U* rhs) noexcept {
  return lhs.Get() == rhs;
}

template <typename T, typename U>
inline bool operator!=(const ThreadLocalPtrProxy<T>& lhs, U* rhs) noexcept {
  return lhs.Get() != rhs;
}

template <typename T, typename U>
inline bool operator==(T* lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs == rhs.Get();
}

template <typename T, typename U>
inline bool operator!=(T* lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs != rhs.Get();
}

template <typename T>
inline bool operator==(const ThreadLocalPtrProxy<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() == nullptr;
}

template <typename T>
inline bool operator==(std::nullptr_t, const ThreadLocalPtrProxy<T>& rhs) noexcept {
  return rhs.Get() == nullptr;
}

template <typename T>
inline bool operator!=(const ThreadLocalPtrProxy<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const ThreadLocalPtrProxy<T>& rhs) noexcept {
  return rhs.Get() != nullptr;
}

template <typename T, typename U>
inline bool operator<(const ThreadLocalPtrProxy<T>& lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs.Get() < rhs.Get();
}

template <typename T, typename U>
inline bool operator>(const ThreadLocalPtrProxy<T>& lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs.Get() > rhs.Get();
}

template <typename T, typename U>
inline bool operator<(const ThreadLocalPtrProxy<T>& lhs, const U* rhs) noexcept {
  return lhs.Get() < rhs;
}

template <typename T, typename U>
inline bool operator<(T* lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs < rhs.Get();
}

template <typename T, typename U>
inline bool operator>(const ThreadLocalPtrProxy<T>& lhs, const U* rhs) noexcept {
  return lhs.Get() > rhs;
}

template <typename T, typename U>
inline bool operator>(T* lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs > rhs.Get();
}

template <typename T>
inline bool operator<(const ThreadLocalPtrProxy<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() < nullptr;
}

template <typename T>
inline bool operator<(std::nullptr_t, const ThreadLocalPtrProxy<T>& rhs) noexcept {
  return nullptr < rhs.Get();
}

template <typename T>
inline bool operator>(const ThreadLocalPtrProxy<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() > nullptr;
}

template <typename T>
inline bool operator>(std::nullptr_t, const ThreadLocalPtrProxy<T>& rhs) noexcept {
  return nullptr > rhs.Get();
}

template <typename T, typename U>
inline bool operator>=(const ThreadLocalPtrProxy<T>& lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs.Get() >= rhs.Get();
}

template <typename T, typename U>
inline bool operator<=(const ThreadLocalPtrProxy<T>& lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs.Get() <= rhs.Get();
}

template <typename T, typename U>
inline bool operator<=(const ThreadLocalPtrProxy<T>& lhs, const U* rhs) noexcept {
  return lhs.Get() <= rhs;
}

template <typename T, typename U>
inline bool operator<=(T* lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs <= rhs.Get();
}

template <typename T, typename U>
inline bool operator>=(const ThreadLocalPtrProxy<T>& lhs, const U* rhs) noexcept {
  return lhs.Get() >= rhs;
}

template <typename T, typename U>
inline bool operator>=(T* lhs, const ThreadLocalPtrProxy<U>& rhs) noexcept {
  return lhs >= rhs.Get();
}

template <typename T>
inline bool operator<=(const ThreadLocalPtrProxy<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() <= nullptr;
}

template <typename T>
inline bool operator<=(std::nullptr_t, const ThreadLocalPtrProxy<T>& rhs) noexcept {
  return nullptr <= rhs.Get();
}

template <typename T>
inline bool operator>=(const ThreadLocalPtrProxy<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() >= nullptr;
}

template <typename T>
inline bool operator>=(std::nullptr_t, const ThreadLocalPtrProxy<T>& rhs) noexcept {
  return nullptr >= rhs.Get();
}
}  // namespace yaclib::detail::fiber

#pragma once

namespace yaclib::detail::fiber {

void* GetImpl(std::uint64_t i);

void Set(void* new_value, std::uint64_t i);

void SetDefault(void* new_value, std::uint64_t i);

template <typename Type>
class ThreadLocalPtrProxy final {
  inline static std::uint64_t sNextFreeIndex = 0;

 public:
  ThreadLocalPtrProxy() noexcept : _i(sNextFreeIndex++) {
  }

  ThreadLocalPtrProxy(Type* value) noexcept : _i(sNextFreeIndex++) {
    if (value != nullptr) {
      SetDefault(value, _i);
    }
  }
  ThreadLocalPtrProxy(ThreadLocalPtrProxy&& other) noexcept : _i(other._i) {
  }
  ThreadLocalPtrProxy(const ThreadLocalPtrProxy& other) noexcept : _i(sNextFreeIndex++) {
    SetDefault(GetImpl(other._i), _i);
  }

  ThreadLocalPtrProxy& operator=(Type* value) noexcept {
    Set(value, this->_i);
    return *this;
  }
  ThreadLocalPtrProxy& operator=(ThreadLocalPtrProxy&& other) noexcept {
    _i = other._i;
    return *this;
  }
  ThreadLocalPtrProxy& operator=(const ThreadLocalPtrProxy& other) noexcept {
    if (this->Get() == other.Get()) {
      return *this;
    }
    SetDefault(GetImpl(other._i), _i);
    return *this;
  }

  template <typename U>
  ThreadLocalPtrProxy(ThreadLocalPtrProxy<U>&& other) noexcept : _i(other._i) {
  }
  template <typename U>
  ThreadLocalPtrProxy(const ThreadLocalPtrProxy<U>& other) noexcept : _i(sNextFreeIndex++) {
    SetDefault(GetImpl(other._i), _i);
  }

  template <typename U>
  ThreadLocalPtrProxy& operator=(ThreadLocalPtrProxy<U>&& other) noexcept {
    _i = other._i;
    return *this;
  }
  template <typename U>
  ThreadLocalPtrProxy& operator=(const ThreadLocalPtrProxy<U>& other) noexcept {
    SetDefault(GetImpl(other._i), _i);
    return *this;
  }

  Type& operator*() const noexcept {
    auto& val = *(Get());
    return val;
  }

  Type* operator->() const noexcept {
    return Get();
  }

  Type* Get() const noexcept {
    return static_cast<Type*>(GetImpl(this->_i));
  }

  explicit operator bool() const noexcept {
    return Get() != nullptr;
  }

  Type& operator[](std::size_t i) const {
    auto* val = Get();
    val += i;
    return *val;
  }

 private:
  std::uint64_t _i;
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

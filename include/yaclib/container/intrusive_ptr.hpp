#pragma once

#include <yaclib/ref.hpp>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace yaclib::container::intrusive {

template <typename T>
class Ptr {
  static_assert(std::is_base_of_v<IRef, T>, "T must be derived class of yaclib::IRef");

 public:
  constexpr Ptr() noexcept : _ptr{nullptr} {
  }

  Ptr(T* other, bool acquire = true) noexcept : _ptr{other} {
    if (_ptr && acquire) {
      _ptr->IncRef();
    }
  }
  Ptr(Ptr&& other) noexcept : _ptr{std::exchange(other._ptr, nullptr)} {
  }
  Ptr(const Ptr& other) noexcept : _ptr{other._ptr} {
    if (_ptr) {
      _ptr->IncRef();
    }
  }
  template <typename U>
  Ptr(Ptr<U>&& other) noexcept : _ptr{std::exchange(other._ptr, nullptr)} {
  }
  template <typename U>
  Ptr(const Ptr<U>& other) noexcept : _ptr{other._ptr} {
    if (_ptr) {
      _ptr->IncRef();
    }
  }

  Ptr& operator=(T* other) noexcept {
    Ptr{other}.Swap(*this);
    return *this;
  }
  Ptr& operator=(Ptr&& other) noexcept {
    Swap(other);
    return *this;
  }
  Ptr& operator=(const Ptr& other) noexcept {
    Ptr{other}.Swap(*this);
    return *this;
  }
  template <typename U>
  Ptr& operator=(const Ptr<U>& other) noexcept {
    Ptr{other}.Swap(*this);
    return *this;
  }
  template <typename U>
  Ptr& operator=(Ptr<U>&& other) noexcept {
    Ptr{std::move(other)}.Swap(*this);
    return *this;
  }

  ~Ptr() {
    if (_ptr) {
      _ptr->DecRef();
    }
  }

  T* Get() const noexcept {
    return _ptr;
  }

  T* Release() {
    return std::exchange(_ptr, nullptr);
  }

  T& operator*() const noexcept {
    // TODO(MBkkt): assert(_ptr != nullptr);
    return *_ptr;
  }

  T* operator->() const noexcept {
    // TODO(MBkkt): assert(_ptr != nullptr);
    return _ptr;
  }

  explicit operator bool() const noexcept {
    return _ptr != nullptr;
  }

  inline void Swap(Ptr& other) noexcept {
    std::swap(_ptr, other._ptr);
  }

 private:
  T* _ptr;

  template <typename U>
  friend class Ptr;
};

template <typename T, typename U>
inline bool operator==(const Ptr<T>& lhs, const Ptr<U>& rhs) noexcept {
  return lhs.Get() == rhs.Get();
}
template <typename T, typename U>
inline bool operator!=(const Ptr<T>& lhs, const Ptr<U>& rhs) noexcept {
  return lhs.Get() != rhs.Get();
}

template <typename T, typename U>
inline bool operator==(const Ptr<T>& lhs, U* rhs) noexcept {
  return lhs.Get() == rhs;
}

template <typename T, typename U>
inline bool operator!=(const Ptr<T>& lhs, U* rhs) noexcept {
  return lhs.Get() != rhs;
}

template <typename T, typename U>
inline bool operator==(T* lhs, const Ptr<U>& rhs) noexcept {
  return lhs == rhs.Get();
}

template <typename T, typename U>
inline bool operator!=(T* lhs, const Ptr<U>& rhs) noexcept {
  return lhs != rhs.Get();
}
template <typename T>
inline bool operator==(const Ptr<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() == nullptr;
}

template <typename T>
inline bool operator==(std::nullptr_t, const Ptr<T>& rhs) noexcept {
  return rhs.Get() == nullptr;
}

template <typename T>
inline bool operator!=(const Ptr<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const Ptr<T>& rhs) noexcept {
  return rhs.Get() != nullptr;
}

template <typename T>
inline bool operator<(const Ptr<T>& lhs, const Ptr<T>& rhs) noexcept {
  return std::less<T*>{}(lhs.Get(), rhs.Get());
}

template <typename T>
inline void swap(const Ptr<T>& lhs, const Ptr<T>& rhs) noexcept {
  lhs.Swap(rhs);
}

}  // namespace yaclib::container::intrusive

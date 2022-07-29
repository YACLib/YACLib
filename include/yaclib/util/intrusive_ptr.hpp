#pragma once

#include <yaclib/util/ref.hpp>

#include <type_traits>

namespace yaclib {

struct NoRefTag final {};

/**
 * A intrusive pointer to objects with an embedded reference count
 *
 * https://www.boost.org/doc/libs/1_77_0/libs/smart_ptr/doc/html/smart_ptr.html#intrusive_ptr
 */
template <typename T>
class IntrusivePtr final {
  static_assert(std::is_base_of_v<IRef, T>, "T must be derived class of IRef");

 public:
  IntrusivePtr() noexcept;

  IntrusivePtr(T* other) noexcept;
  IntrusivePtr(IntrusivePtr&& other) noexcept;
  IntrusivePtr(const IntrusivePtr& other) noexcept;

  IntrusivePtr& operator=(T* other) noexcept;
  IntrusivePtr& operator=(IntrusivePtr&& other) noexcept;
  IntrusivePtr& operator=(const IntrusivePtr& other) noexcept;

  template <typename U>
  IntrusivePtr(IntrusivePtr<U>&& other) noexcept;
  template <typename U>
  IntrusivePtr(const IntrusivePtr<U>& other) noexcept;

  template <typename U>
  IntrusivePtr& operator=(IntrusivePtr<U>&& other) noexcept;
  template <typename U>
  IntrusivePtr& operator=(const IntrusivePtr<U>& other) noexcept;

  ~IntrusivePtr() noexcept;

  T* Get() const noexcept;
  T* Release() noexcept;

  explicit operator bool() const noexcept;

  T& operator*() const noexcept;
  T* operator->() const noexcept;

  void Swap(IntrusivePtr& other) noexcept;

  IntrusivePtr(NoRefTag, T* other) noexcept;
  void Reset(NoRefTag, T* other) noexcept;

 private:
  template <typename U>
  friend class IntrusivePtr;

  T* _ptr;
};

template <typename T, typename U>
inline bool operator==(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept {
  return lhs.Get() == rhs.Get();
}

template <typename T, typename U>
inline bool operator!=(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept {
  return lhs.Get() != rhs.Get();
}

template <typename T, typename U>
inline bool operator==(const IntrusivePtr<T>& lhs, U* rhs) noexcept {
  return lhs.Get() == rhs;
}

template <typename T, typename U>
inline bool operator!=(const IntrusivePtr<T>& lhs, U* rhs) noexcept {
  return lhs.Get() != rhs;
}

template <typename T, typename U>
inline bool operator==(T* lhs, const IntrusivePtr<U>& rhs) noexcept {
  return lhs == rhs.Get();
}

template <typename T, typename U>
inline bool operator!=(T* lhs, const IntrusivePtr<U>& rhs) noexcept {
  return lhs != rhs.Get();
}

template <typename T>
inline bool operator==(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() == nullptr;
}

template <typename T>
inline bool operator==(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept {
  return rhs.Get() == nullptr;
}

template <typename T>
inline bool operator!=(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept {
  return lhs.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept {
  return rhs.Get() != nullptr;
}

template <typename T>
inline bool operator<(const IntrusivePtr<T>& lhs, const IntrusivePtr<T>& rhs) noexcept {
  return lhs.Get() < rhs.Get();
}

}  // namespace yaclib

#include <yaclib/util/detail/intrusive_ptr_impl.hpp>

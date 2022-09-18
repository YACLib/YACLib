#pragma once

#include <yaclib/log.hpp>

namespace yaclib {

template <typename T>
IntrusivePtr<T>::IntrusivePtr() noexcept : _ptr{nullptr} {
}

template <typename T>
IntrusivePtr<T>::IntrusivePtr(T* other) noexcept : _ptr{other} {
  if (_ptr) {
    _ptr->IncRef();
  }
}

template <typename T>
IntrusivePtr<T>::IntrusivePtr(IntrusivePtr&& other) noexcept : _ptr{other._ptr} {
  other._ptr = nullptr;
}

template <typename T>
IntrusivePtr<T>::IntrusivePtr(const IntrusivePtr& other) noexcept : IntrusivePtr{other._ptr} {
}

template <typename T>
IntrusivePtr<T>& IntrusivePtr<T>::operator=(T* other) noexcept {
  if (_ptr != other) {
    IntrusivePtr{other}.Swap(*this);
  }
  return *this;
}

template <typename T>
IntrusivePtr<T>& IntrusivePtr<T>::operator=(IntrusivePtr&& other) noexcept {
  Swap(other);
  return *this;
}

template <typename T>
IntrusivePtr<T>& IntrusivePtr<T>::operator=(const IntrusivePtr& other) noexcept {
  return operator=(other._ptr);
}

template <typename T>
template <typename U>
IntrusivePtr<T>::IntrusivePtr(IntrusivePtr<U>&& other) noexcept : _ptr{other._ptr} {
  other._ptr = nullptr;
}

template <typename T>
template <typename U>
IntrusivePtr<T>::IntrusivePtr(const IntrusivePtr<U>& other) noexcept : IntrusivePtr{other._ptr} {
}

template <typename T>
template <typename U>
IntrusivePtr<T>& IntrusivePtr<T>::operator=(IntrusivePtr<U>&& other) noexcept {
  IntrusivePtr{std::move(other)}.Swap(*this);
  return *this;
}

template <typename T>
template <typename U>
IntrusivePtr<T>& IntrusivePtr<T>::operator=(const IntrusivePtr<U>& other) noexcept {
  return operator=(other._ptr);
}

template <typename T>
IntrusivePtr<T>::~IntrusivePtr() noexcept {
  if (_ptr) {
    _ptr->DecRef();
  }
}

template <typename T>
T* IntrusivePtr<T>::Get() const noexcept {
  return _ptr;
}

template <typename T>
T* IntrusivePtr<T>::Release() noexcept {
  auto ptr = _ptr;
  _ptr = nullptr;
  return ptr;
}

template <typename T>
IntrusivePtr<T>::operator bool() const noexcept {
  return _ptr != nullptr;
}

template <typename T>
T& IntrusivePtr<T>::operator*() const noexcept {
  YACLIB_ASSERT(_ptr != nullptr);
  return *_ptr;
}

template <typename T>
T* IntrusivePtr<T>::operator->() const noexcept {
  YACLIB_ASSERT(_ptr != nullptr);
  return _ptr;
}

template <typename T>
void IntrusivePtr<T>::Swap(IntrusivePtr& other) noexcept {
  auto* ptr = _ptr;
  _ptr = other._ptr;
  other._ptr = ptr;
}

template <typename T>
IntrusivePtr<T>::IntrusivePtr(NoRefTag, T* other) noexcept : _ptr{other} {
}

template <typename T>
void IntrusivePtr<T>::Reset(NoRefTag, T* other) noexcept {
  _ptr = other;
}

}  // namespace yaclib

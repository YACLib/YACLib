#pragma once

#include <type_traits>

namespace yaclib {

template <typename V, typename E>
template <typename T>
void Promise<V, E>::Set(T&& object) && {
  static_assert(std::is_constructible_v<Result<V, E>, T>, "TODO(MBkkt): Add message");
  _core.Release()->Done(std::forward<T>(object), [] {
  });
}

template <typename V, typename E>
void Promise<V, E>::Set() && noexcept {
  static_assert(std::is_void_v<V>);
  _core.Release()->Done(Unit{}, [] {
  });
}

template <typename V, typename E>
Promise<V, E>::Promise(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& Promise<V, E>::GetCore() noexcept {
  return _core;
}

template <typename V, typename E>
Promise<V, E>::~Promise() noexcept {
  if (_core) {
    _core.Release()->Done(StopTag{}, [] {
    });
  }
}

template <typename V, typename E>
bool Promise<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

}  // namespace yaclib

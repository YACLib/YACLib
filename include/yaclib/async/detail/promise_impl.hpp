#pragma once

#include <yaclib/config.hpp>

#include <type_traits>

namespace yaclib {

template <typename V, typename E>
template <typename Type>
void Promise<V, E>::Set(Type&& value) && {
  static_assert(std::is_constructible_v<Result<V, E>, Type>, "TODO(MBkkt): Add message");
  std::exchange(_core, nullptr)->Set(std::forward<Type>(value));
}

template <typename V, typename E>
void Promise<V, E>::Set() && {
  static_assert(std::is_void_v<V>);
  std::exchange(_core, nullptr)->Set(Unit{});
}

template <typename V, typename E>
Promise<V, E>::Promise(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}
template <typename V, typename E>
detail::ResultCorePtr<V, E>& Promise<V, E>::GetCore() noexcept {
  return _core;
}
template <typename V, typename E>
Promise<V, E>::~Promise() {
  if (_core) {
    _core->Set(StopTag{});
  }
}

template <typename V, typename E>
bool Promise<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

}  // namespace yaclib

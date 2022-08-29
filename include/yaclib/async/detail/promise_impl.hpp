#pragma once

#include <type_traits>

namespace yaclib {

template <typename V, typename E>
template <typename T>
void Promise<V, E>::Set(T&& object) && {
  static_assert(std::is_constructible_v<Result<V, E>, T>, "TODO(MBkkt): Add message");
  YACLIB_ASSERT(Valid());
  auto& core = *_core.Release();
  core.Store(std::forward<T>(object));
  core.template SetResult<false>();
}

template <typename V, typename E>
void Promise<V, E>::Set() && noexcept {
  static_assert(std::is_void_v<V>);
  std::move(*this).Set(Unit{});
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
    std::move(*this).Set(StopTag{});
  }
}

template <typename V, typename E>
bool Promise<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

}  // namespace yaclib

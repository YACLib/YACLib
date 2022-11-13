#pragma once

#include <type_traits>

namespace yaclib {

template <typename V, typename E>
template <typename... Args>
void Promise<V, E>::Set(Args&&... args) && {
  YACLIB_ASSERT(Valid());
  if constexpr (sizeof...(Args) == 0) {
    _core->Store(std::in_place);
  } else {
    _core->Store(std::forward<Args>(args)...);
  }
  auto* core = _core.Release();
  detail::Loop(core, core->template SetResult<false>());
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

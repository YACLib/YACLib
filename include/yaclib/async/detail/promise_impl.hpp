#pragma once

#ifndef YACLIB_ASYNC_IMPL
#error "You can not include this header directly, use yaclib/async/promise.hpp"
#endif

namespace yaclib {

template <typename T>
Promise<T>::Promise() : Promise(util::MakeIntrusive<detail::ResultCore<T>>()) {
}

template <typename T>
Future<T> Promise<T>::MakeFuture() {
  assert(_core != nullptr);
  assert(static_cast<util::Counter<detail::ResultCore<T>>&>(*_core).GetRef());
  return Future<T>{_core};
}

template <typename T>
template <typename Type>
void Promise<T>::Set(Type&& value) && {
  static_assert(std::is_constructible_v<util::Result<T>, Type>, "TODO(MBkkt): Add message");
  std::exchange(_core, nullptr)->Set(std::forward<Type>(value));
}

template <typename T>
void Promise<T>::Set() && {
  static_assert(std::is_void_v<T>);
  std::exchange(_core, nullptr)->Set(util::Unit{});
}

template <typename T>
Promise<T>::Promise(detail::PromiseCorePtr<T> core) noexcept : _core{std::move(core)} {
}
template <typename T>
detail::PromiseCorePtr<T>& Promise<T>::GetCore() noexcept {
  return _core;
}
template <typename T>
Promise<T>::~Promise() {
  if (_core) {
    _core->Set(std::error_code{});
  }
}

template <typename T>
bool Promise<T>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename T>
Contract<T> MakeContract() {
  Promise<T> p;
  auto f{p.MakeFuture()};
  return {std::move(f), std::move(p)};
}

}  // namespace yaclib

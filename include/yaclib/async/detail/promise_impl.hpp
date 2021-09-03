#pragma once

#ifndef ASYNC_IMPL
#error "You can not include this header directly, use yaclib/async/async.hpp"
#endif

namespace yaclib::async {

template <typename T>
Promise<T>::Promise() : _core{new container::Counter<detail::Core<T, void, void>>{}} {
}

template <typename T>
Future<T> Promise<T>::MakeFuture() {
  assert(!_future_extracted);
  return Future<T>{_core};
}

template <typename T>
template <typename Type>
void Promise<T>::Set(Type&& value) && {
  static_assert(!std::is_void_v<T> || std::is_same_v<std::error_code, util::DecayT<Type>> ||
                std::is_same_v<std::exception_ptr, util::DecayT<Type>> ||
                std::is_same_v<util::Result<void>, util::DecayT<Type>>);
  _core->SetResult({std::forward<Type>(value)});
}

template <typename T>
void Promise<T>::Set() && {
  static_assert(std::is_void_v<T>);
  _core->SetResult(util::Result<T>::Default());
}

template <typename T>
Contract<T> MakeContract() {
  Promise<T> p;
  auto f{p.MakeFuture()};
  return {std::move(f), std::move(p)};
}

}  // namespace yaclib::async

#pragma once

#include <yaclib/async/connect.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/shared_contract.hpp>

namespace yaclib {

template <typename V, typename T>
SharedFuture<V, T> Split(FutureBase<V, T>&& future) {
  static_assert(std::is_copy_constructible_v<wrap_void_t<V>>, "Cannot split this Result");
  auto [f, p] = MakeSharedContract<V, T>();
  Connect(std::move(future), std::move(p));
  return std::move(f);
}

template <typename V, typename T>
SharedFuture<V, T> Split(SharedPromise<V, T>& promise) {
  YACLIB_ASSERT(promise.Valid());
  return SharedFuture<V, T>{promise.GetCore()};
}

}  // namespace yaclib

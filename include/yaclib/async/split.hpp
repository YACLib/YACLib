#pragma once

#include <yaclib/async/connect.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/shared_contract.hpp>

namespace yaclib {

template <typename V, typename E>
SharedFuture<V, E> Split(FutureBase<V, E>&& future) {
  static_assert(std::is_copy_constructible_v<Result<V, E>>, "Cannot split this Result<V, E>");
  auto [f, p] = MakeSharedContract<V, E>();
  Connect(std::move(future), std::move(p));
  return std::move(f);
}

template <typename V, typename E>
SharedFuture<V, E> Split(SharedPromise<V, E>& promise) {
  YACLIB_ASSERT(promise.Valid());
  return SharedFuture<V, E>(promise.GetCore());
}

}  // namespace yaclib

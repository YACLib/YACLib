#pragma once

#include <yaclib/async/connect.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {

template <typename V, typename E>
Future<V, E> Share(const SharedFuture<V, E>& future) {
  auto [f, p] = MakeContract<V, E>();
  Connect(future, std::move(p));
  return std::move(f);
}

template <typename V, typename E>
FutureOn<V, E> Share(const SharedFuture<V, E>& future, IExecutor& executor) {
  auto [f, p] = MakeContractOn<V, E>(executor);
  Connect(future, std::move(p));
  return std::move(f);
}

template <typename V, typename E>
Future<V, E> Share(SharedPromise<V, E>& promise) {
  YACLIB_ASSERT(promise.Valid());
  auto [f, p] = MakeContract<V, E>();
  Connect(promise, std::move(p));
  return std::move(f);
}

template <typename V, typename E>
FutureOn<V, E> Share(SharedPromise<V, E>& promise, IExecutor& executor) {
  YACLIB_ASSERT(promise.Valid());
  auto [f, p] = MakeContractOn<V, E>(executor);
  Connect(promise, std::move(p));
  return std::move(f);
}

}  // namespace yaclib

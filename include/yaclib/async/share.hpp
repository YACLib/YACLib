#pragma once

#include <yaclib/async/connect.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {

template <typename V, typename T>
Future<V, T> Share(const SharedFutureBase<V, T>& future) {
  auto [f, p] = MakeContract<V, T>();
  Connect(future, std::move(p));
  return std::move(f);
}

template <typename V, typename T>
FutureOn<V, T> Share(const SharedFutureBase<V, T>& future, IExecutor& executor) {
  auto [f, p] = MakeContractOn<V, T>(executor);
  Connect(future, std::move(p));
  return std::move(f);
}

template <typename V, typename T>
Future<V, T> Share(SharedPromise<V, T>& promise) {
  YACLIB_ASSERT(promise.Valid());
  auto [f, p] = MakeContract<V, T>();
  Connect(promise, std::move(p));
  return std::move(f);
}

template <typename V, typename T>
FutureOn<V, T> Share(SharedPromise<V, T>& promise, IExecutor& executor) {
  YACLIB_ASSERT(promise.Valid());
  auto [f, p] = MakeContractOn<V, T>(executor);
  Connect(promise, std::move(p));
  return std::move(f);
}

}  // namespace yaclib

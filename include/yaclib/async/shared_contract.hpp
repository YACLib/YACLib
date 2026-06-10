#pragma once

#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/util/helper.hpp>

#include <utility>

namespace yaclib {

template <typename V, typename T>
using SharedContract = std::pair<SharedFuture<V, T>, SharedPromise<V, T>>;

template <typename V, typename T>
using SharedContractOn = std::pair<SharedFutureOn<V, T>, SharedPromise<V, T>>;

template <typename V = void, typename T = DefaultTrait>
[[nodiscard]] SharedContract<V, T> MakeSharedContract() {
  auto core = MakeShared<detail::SharedCore<V, T>>(detail::kSharedRefWithFuture);
  SharedFuture<V, T> future{detail::SharedCorePtr<V, T>{NoRefTag{}, core.Get()}};
  SharedPromise<V, T> promise{detail::SharedCorePtr<V, T>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename T = DefaultTrait>
[[nodiscard]] SharedContract<V, T> MakeSharedContractOn(IExecutor& e) {
  auto core = MakeShared<detail::SharedCore<V, T>>(detail::kSharedRefWithFuture);
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  SharedFutureOn<V, T> future{detail::SharedCorePtr<V, T>{NoRefTag{}, core.Get()}};
  SharedPromise<V, T> promise{detail::SharedCorePtr<V, T>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename T = DefaultTrait>
[[nodiscard]] SharedPromise<V, T> MakeSharedPromise() {
  auto core = MakeShared<detail::SharedCore<V, T>>(detail::kSharedRefNoFuture);
  SharedPromise<V, T> promise{detail::SharedCorePtr<V, T>{NoRefTag{}, core.Release()}};
  return promise;
}

}  // namespace yaclib

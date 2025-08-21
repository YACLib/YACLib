#pragma once

#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/util/helper.hpp>

#include <utility>

namespace yaclib {

template <typename V, typename E>
using SharedContract = std::pair<SharedFuture<V, E>, SharedPromise<V, E>>;

template <typename V, typename E>
using SharedContractOn = std::pair<SharedFutureOn<V, E>, SharedPromise<V, E>>;

template <typename V = void, typename E = StopError>
[[nodiscard]] SharedContract<V, E> MakeSharedContract() {
  auto core = MakeShared<detail::SharedCore<V, E>>(detail::kSharedRefWithFuture);
  SharedFuture<V, E> future{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Get()}};
  SharedPromise<V, E> promise{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename E = StopError>
[[nodiscard]] SharedContract<V, E> MakeSharedContractOn(IExecutor& e) {
  auto core = MakeShared<detail::SharedCore<V, E>>(detail::kSharedRefWithFuture);
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  SharedFutureOn<V, E> future{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Get()}};
  SharedPromise<V, E> promise{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename E = StopError>
[[nodiscard]] SharedPromise<V, E> MakeSharedPromise() {
  auto core = MakeShared<detail::SharedCore<V, E>>(detail::kSharedRefNoFuture);
  SharedPromise<V, E> promise{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return promise;
}

}  // namespace yaclib

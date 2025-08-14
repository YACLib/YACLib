#pragma once

#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/util/helper.hpp>

#include <utility>

namespace yaclib {

template <typename V, typename E>
using SharedContract = std::pair<SharedFuture<V, E>, SharedPromise<V, E>>;

template <typename V = void, typename E = StopError>
[[nodiscard]] SharedContract<V, E> MakeSharedContract() {
  // 3 refs for the promise (1 for the promise itself and 2 for the last callback)
  // 1 ref for the future
  auto core = MakeShared<detail::SharedCore<V, E>>(4);
  SharedFuture<V, E> future{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Get()}};
  SharedPromise<V, E> promise{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename E = StopError>
[[nodiscard]] SharedPromise<V, E> MakeSharedPromise() {
  auto core = MakeShared<detail::SharedCore<V, E>>(3);
  SharedPromise<V, E> promise{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return promise;
}

}  // namespace yaclib

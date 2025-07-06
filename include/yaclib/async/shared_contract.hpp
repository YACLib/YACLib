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
  // 2 refs for SharedPromise, 1 ref for SharedFuture
  auto core = MakeShared<detail::SharedCore<V, E>>(3);
  SharedFuture<V, E> future{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Get()}};
  SharedPromise<V, E> promise{detail::SharedCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

}  // namespace yaclib

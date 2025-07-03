#pragma once

#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>

namespace yaclib {

template <typename V, typename E>
using SharedContract = std::pair<SharedFuture<V, E>, SharedPromise<V, E>>;

template <typename V = void, typename E = StopError>
[[nodiscard]] SharedContract<V, E> MakeSharedContract() {
  auto core = MakeShared<detail::SharedCore<V, E>>(1);
  SharedFuture<V, E> future{core};
  SharedPromise<V, E> promise{core};
  return {std::move(future), std::move(promise)};
}

}  // namespace yaclib

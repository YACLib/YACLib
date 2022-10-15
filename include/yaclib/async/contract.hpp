#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {

/**
 * Describes channel with future and promise
 */
template <typename V, typename E>
using Contract = std::pair<Future<V, E>, Promise<V, E>>;

template <typename V, typename E>
using ContractOn = std::pair<FutureOn<V, E>, Promise<V, E>>;
// TODO(kononovk) Make Contract a struct, not std::pair or std::tuple

/**
 * Creates related future and promise
 *
 * \return a \see Contract object with new future and promise
 */
template <typename V = void, typename E = StopError>
[[nodiscard]] Contract<V, E> MakeContract() {
  auto core = MakeUnique<detail::ResultCore<V, E>>();
  Future<V, E> future{detail::ResultCorePtr<V, E>{NoRefTag{}, core.Get()}};
  Promise<V, E> promise{detail::ResultCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename E = StopError>
[[nodiscard]] ContractOn<V, E> MakeContractOn(IExecutor& e) {
  auto core = MakeUnique<detail::ResultCore<V, E>>();
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  FutureOn<V, E> future{detail::ResultCorePtr<V, E>{NoRefTag{}, core.Get()}};
  Promise<V, E> promise{detail::ResultCorePtr<V, E>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

}  // namespace yaclib

#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {

/**
 * Describes channel with future and promise
 */
template <typename V, typename T>
using Contract = std::pair<Future<V, T>, Promise<V, T>>;

template <typename V, typename T>
using ContractOn = std::pair<FutureOn<V, T>, Promise<V, T>>;
// TODO(kononovk) Make Contract a struct, not std::pair or std::tuple

/**
 * Creates related future and promise
 *
 * \return a \see Contract object with new future and promise
 */
template <typename V = void, typename T = DefaultTrait>
[[nodiscard]] Contract<V, T> MakeContract() {
  auto core = MakeUnique<detail::ResultCore<V, T>>();
  Future<V, T> future{detail::ResultCorePtr<V, T>{NoRefTag{}, core.Get()}};
  Promise<V, T> promise{detail::ResultCorePtr<V, T>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

template <typename V = void, typename T = DefaultTrait>
[[nodiscard]] ContractOn<V, T> MakeContractOn(IExecutor& e) {
  auto core = MakeUnique<detail::ResultCore<V, T>>();
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  FutureOn<V, T> future{detail::ResultCorePtr<V, T>{NoRefTag{}, core.Get()}};
  Promise<V, T> promise{detail::ResultCorePtr<V, T>{NoRefTag{}, core.Release()}};
  return {std::move(future), std::move(promise)};
}

}  // namespace yaclib

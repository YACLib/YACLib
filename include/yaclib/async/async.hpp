#pragma once

#include <yaclib/async/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/executor/executor.hpp>

#include <type_traits>

namespace yaclib::async {

/**
 * Execute Callable functor via executor
 */
template <typename Functor>
auto AsyncVia(executor::IExecutorPtr executor, Functor&& functor) {
  using Result = decltype(functor());

  using CoreType = Core<Result, std::decay_t<Functor>, void>;
  auto shared_state = container::intrusive::Ptr{
      new container::Counter<CoreType>{std::forward<Functor>(functor)}};

  shared_state->SetExecutor(executor);
  executor->Execute(*shared_state);
  return Future<Result>{shared_state};
}

}  // namespace yaclib::async

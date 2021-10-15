#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/executor/executor.hpp>

#include <type_traits>

namespace yaclib {

/**
 * Execute Callable functor via executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f functor to execute
 * \return \ref Future corresponding f return value
 */
template <typename Functor>
auto Run(const IExecutorPtr& e, Functor&& f) {
  using Ret = detail::Return<void, Functor, 2>;
  using U = typename Ret::Type;
  if constexpr (Ret::kIsAsync) {
    using InvokeT = detail::AsyncInvoke<U, decltype(std::forward<Functor>(f)), void>;
    using CoreType = detail::Core<void, InvokeT, void>;
    auto [future, promise] = MakeContract<U>();
    future._core->SetExecutor(e);
    util::Ptr core{new util::Counter<CoreType>{e, std::move(promise), std::forward<Functor>(f)}};
    core->SetExecutor(e);
    e->Execute(*core);
    return std::move(future);
  } else {
    using InvokeT = detail::Invoke<U, decltype(std::forward<Functor>(f)), void>;
    using CoreType = detail::Core<U, InvokeT, void>;
    util::Ptr core{new util::Counter<CoreType>{std::forward<Functor>(f)}};
    core->SetExecutor(e);
    e->Execute(*core);
    return Future<U>{core};
  }
}

}  // namespace yaclib

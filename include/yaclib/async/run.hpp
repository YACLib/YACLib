#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/detail/future.hpp>
#include <yaclib/async/detail/promise.hpp>
#include <yaclib/executor/executor.hpp>

#include <type_traits>

#define YACLIB_ASYNC_IMPL
#include <yaclib/async/detail/future_impl.hpp>
#include <yaclib/async/detail/promise_impl.hpp>
#undef YACLIB_ASYNC_IMPL

namespace yaclib::async {

/**
 * Execute Callable functor via executor
 */
template <typename Functor>
auto Run(const executor::IExecutorPtr& e, Functor&& f) {
  using Ret = detail::Return<void, Functor, 2>;
  using U = typename Ret::Type;
  if constexpr (Ret::kIsAsync) {
    using InvokeT = detail::AsyncInvoke<U, decltype(std::forward<Functor>(f)), void>;
    using CoreType = detail::Core<void, InvokeT, void>;
    auto [future, promise] = async::MakeContract<U>();
    future._core->SetExecutor(e);
    container::intrusive::Ptr core{new container::Counter<CoreType>{e, std::move(promise), std::forward<Functor>(f)}};
    core->SetExecutor(e);
    e->Execute(*core);
    return std::move(future);
  } else {
    using InvokeT = detail::Invoke<U, decltype(std::forward<Functor>(f)), void>;
    using CoreType = detail::Core<U, InvokeT, void>;
    container::intrusive::Ptr core{new container::Counter<CoreType>{std::forward<Functor>(f)}};
    core->SetExecutor(e);
    e->Execute(*core);
    return Future<U>{core};
  }
}

}  // namespace yaclib::async

#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/type_traits.hpp>

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
  assert(e);
  assert(e->Tag() != IExecutor::Type::Inline);
  using AsyncRet = util::detail::ResultValueT<typename detail::Return<void, Functor, 2>::Type>;
  constexpr bool kIsAsync = util::IsFutureV<AsyncRet>;
  using Ret = util::detail::ResultValueT<util::detail::FutureValueT<AsyncRet>>;
  if constexpr (kIsAsync) {
    util::Ptr callback{new util::Counter<detail::ResultCore<Ret>>{}};
    callback->SetExecutor(e);
    using InvokeT = detail::AsyncInvoke<Ret, decltype(std::forward<Functor>(f)), void>;
    using CoreT = detail::Core<void, InvokeT, void, true>;
    util::Ptr core{new util::Counter<CoreT>{callback, std::forward<Functor>(f)}};
    core->SetExecutor(e);
    e->Execute(*core);
    return Future<Ret>{callback};
  } else {
    using InvokeT = detail::SyncInvoke<Ret, decltype(std::forward<Functor>(f)), void>;
    using CoreT = detail::Core<Ret, InvokeT, void, true>;
    util::Ptr core{new util::Counter<CoreT>{std::forward<Functor>(f)}};
    core->SetExecutor(e);
    e->Execute(*core);
    return Future<Ret>{std::move(core)};
  }
}

}  // namespace yaclib

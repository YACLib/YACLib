#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/detail/future.hpp>
#include <yaclib/async/detail/promise.hpp>
#include <yaclib/executor/executor.hpp>

#include <type_traits>

#define ASYNC_IMPL
#include <yaclib/async/detail/future_impl.hpp>
#include <yaclib/async/detail/promise_impl.hpp>
#undef ASYNC_IMPL

namespace yaclib::async {

/**
 * Execute Callable functor via executor
 */
template <typename Functor>
auto Run(executor::IExecutorPtr executor, Functor&& functor) {
  using U = std::invoke_result_t<Functor>;
  using Ret = util::detail::ResultValueT<U>;

  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<void>) mutable -> util::Result<Ret> {
    try {
      if constexpr (std::is_void_v<U>) {
        functor();
        return util::Result<U>::Default();
      } else {
        return {functor()};
      }
    } catch (...) {
      return {std::current_exception()};
    }
  };
  using CoreType = Core<Ret, std::decay_t<decltype(wrapper)>, void>;
  container::intrusive::Ptr shared_core{new container::Counter<CoreType>{std::move(wrapper)}};
  shared_core->SetExecutor(executor);
  executor->Execute(*shared_core);
  return Future<Ret>{shared_core};
}

}  // namespace yaclib::async

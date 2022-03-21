#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Execute Callable functor via executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f functor to execute
 * \return \ref Future corresponding f return value
 */
template <typename E = StopError, typename Functor>
auto Run(const IExecutorPtr& e, Functor&& f) {
  YACLIB_ERROR(e == nullptr, "nullptr executor supplied");
  YACLIB_INFO(e->Tag() == IExecutor::Type::Inline,
              "better way is use ThenInline(...) instead of Then(MakeInline(), ...)");
  using AsyncRet = result_value_t<typename detail::Return<void, E, Functor, 2>::Type>;
  constexpr bool kIsAsync = is_future_v<AsyncRet>;
  using Ret = result_value_t<future_value_t<AsyncRet>>;
  using Invoke = std::conditional_t<kIsAsync,  //
                                    detail::AsyncInvoke<Ret, void, E, decltype(std::forward<Functor>(f))>,
                                    detail::SyncInvoke<Ret, void, E, decltype(std::forward<Functor>(f))>>;
  using Core = detail::Core<Ret, void, E, Invoke, true>;
  auto core = MakeIntrusive<Core>(std::forward<Functor>(f));
  core->SetExecutor(e);
  e->Submit(*core);
  return Future<Ret, E>{std::move(core)};
}

}  // namespace yaclib

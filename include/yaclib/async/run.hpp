#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Execute Callable functor on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f functor to execute
 * \return \ref FutureOn corresponding f return value
 */
template <typename E = StopError, typename Func>
auto Run(IExecutor& e, Func&& f) {
  YACLIB_INFO(e.Tag() == IExecutor::Type::Inline,
              "better way is call functor explicit, and use MakeFuture to create Future with functor result");
  auto* core = detail::MakeCore<detail::CoreType::Run, void, E>(std::forward<Func>(f));
  core->SetExecutor(&e);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  e.Submit(*core);
  return FutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace yaclib

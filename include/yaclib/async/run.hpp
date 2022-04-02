#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
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
  auto* core = detail::MakeCore<detail::CoreType::Run, void, E>(std::forward<Functor>(f));
  core->SetExecutor(e);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  e->Submit(*core);
  return Future{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace yaclib

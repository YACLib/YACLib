#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * Execute Callable func on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f func to execute
 * \return \ref FutureOn corresponding f return value
 */
template <typename E = StopError, typename Func>
auto Run(IExecutor& e, Func&& f) {
  YACLIB_INFO(e.Tag() == IExecutor::Type::Inline,
              "better way is call func explicit, and use MakeFuture to create Future with func result");
  auto* core = detail::MakeCore<detail::CoreType::Run, void, E>(std::forward<Func>(f));
  core->SetExecutor(&e);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  e.Submit(*core);
  return FutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace yaclib

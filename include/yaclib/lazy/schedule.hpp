#pragma once

#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {

template <typename V, typename E, typename Func>
YACLIB_INLINE auto Schedule(IExecutor& e, Func&& f) {
  auto* core = [&] {
    if constexpr (std::is_same_v<V, Unit>) {
      return MakeCore<CoreType::Run, true, void, E>(std::forward<Func>(f));
    } else {
      return MakeUnique<PromiseCore<V, E, Func&&>>(std::forward<Func>(f)).Release();
    }
  }();
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace detail

/**
 * Execute Callable func on Inline executor
 *
 * \param f func to execute
 * \return \ref Future corresponding f return value
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(Func&& f) {
  return detail::Schedule<Unit, E>(MakeInline(), std::forward<Func>(f));
}

/**
 * Execute Callable func on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f func to execute
 * \return \ref FutureOn corresponding f return value
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(IExecutor& e, Func&& f) {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
              "better way is call func explicit and use MakeTask to create Task with func result"
              " or at least use Schedule(func)");
  return detail::Schedule<Unit, E>(e, std::forward<Func>(f));
}

/**
 * Execute Callable func on Inline executor
 *
 * \param f func to execute
 * \return \ref Future corresponding f return value
 */
template <typename V = void, typename E = StopError, typename Func>
/*Task*/ auto LazyContract(Func&& f) {
  return detail::Schedule<V, E>(MakeInline(), std::forward<Func>(f)).On(nullptr);
}

/**
 * Execute Callable func on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f func to execute
 * \return \ref Task corresponding f return value
 */
template <typename V = void, typename E = StopError, typename Func>
/*Task*/ auto LazyContract(IExecutor& e, Func&& f) {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline, "better way is use LazyContract(func)");
  return detail::Schedule<V, E>(e, std::forward<Func>(f));
}

}  // namespace yaclib

#pragma once

#include <yaclib/algo/detail/core.hpp>
#include <yaclib/algo/detail/promise_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {
namespace detail {

template <typename V = Unit, typename E = StopError, typename Func>
YACLIB_INLINE auto Run(IExecutor& e, Func&& f) {
  auto* core = [&] {
    if constexpr (std::is_same_v<V, Unit>) {
      constexpr auto CoreT = CoreType::Run | CoreType::Call | CoreType::ToUnique;
      return MakeCore<CoreT, void, E>(std::forward<Func>(f));
    } else {
      return MakeUnique<PromiseCore<V, E, Func&&, false>>(std::forward<Func>(f)).Release();
    }
  }();
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  e.Submit(*core);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return FutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

template <typename V = Unit, typename E = StopError, typename Func>
YACLIB_INLINE auto RunShared(IExecutor& e, Func&& f) {
  auto* core = [&] {
    if constexpr (std::is_same_v<V, Unit>) {
      constexpr auto CoreT = CoreType::Run | CoreType::Call | CoreType::ToShared;
      return MakeCore<CoreT, void, E>(std::forward<Func>(f));
    } else {
      return MakeShared<PromiseCore<V, E, Func&&, true>>(detail::kSharedRefWithFuture, std::forward<Func>(f)).Release();
    }
  }();
  e.IncRef();
  core->_executor.Reset(NoRefTag{}, &e);
  e.Submit(*core);
  using ResultCoreT = typename std::remove_reference_t<decltype(*core)>::Base;
  return SharedFutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, core}};
}

}  // namespace detail

/**
 * Execute Callable func on Inline executor
 *
 * \param f func to execute
 * \return \ref Future corresponding f return value
 */
template <typename E = StopError, typename Func>
/*Future*/ auto Run(Func&& f) {
  return detail::Run<Unit, E>(MakeInline(), std::forward<Func>(f)).On(nullptr);
}

template <typename E = StopError, typename Func>
/*SharedFuture*/ auto RunShared(Func&& f) {
  return detail::RunShared<Unit, E>(MakeInline(), std::forward<Func>(f));
}

/**
 * Execute Callable func on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f func to execute
 * \return \ref FutureOn corresponding f return value
 */
template <typename E = StopError, typename Func>
/*FutureOn*/ auto Run(IExecutor& e, Func&& f) {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
              "better way is call func explicit and use MakeFuture to create Future with func result"
              " or at least use Run(func)");
  return detail::Run<Unit, E>(e, std::forward<Func>(f));
}

template <typename E = StopError, typename Func>
/*SharedFuture*/ auto RunShared(IExecutor& e, Func&& f) {
  return detail::RunShared<Unit, E>(e, std::forward<Func>(f));
}

/**
 * Execute Callable func on Inline executor
 *
 * \param f func to execute
 * \return \ref Future corresponding f return value
 */
template <typename V = void, typename E = StopError, typename Func>
/*Future*/ auto AsyncContract(Func&& f) {
  return detail::Run<V, E>(MakeInline(), std::forward<Func>(f)).On(nullptr);
}

template <typename V = void, typename E = StopError, typename Func>
/*SharedFuture*/ auto AsyncSharedContract(Func&& f) {
  return detail::RunShared<V, E>(MakeInline(), std::forward<Func>(f));
}

/**
 * Execute Callable func on executor
 *
 * \param e executor to be used to execute f and saved as callback executor for return \ref Future
 * \param f func to execute
 * \return \ref FutureOn corresponding f return value
 */
template <typename V = void, typename E = StopError, typename Func>
/*FutureOn*/ auto AsyncContract(IExecutor& e, Func&& f) {
  YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
              "better way is call func explicit and use MakeFuture to create Future with func result"
              " or at least use AsyncContract(func)");
  return detail::Run<V, E>(e, std::forward<Func>(f));
}

template <typename V = void, typename E = StopError, typename Func>
/*SharedFuture*/ auto AsyncSharedContract(IExecutor& e, Func&& f) {
  return detail::RunShared<V, E>(e, std::forward<Func>(f));
}

}  // namespace yaclib

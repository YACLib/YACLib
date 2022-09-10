#pragma once

#include <yaclib/lazy/task.hpp>

namespace yaclib {

/**
 * TODO(MBkkt) add description
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(IExecutor& e, Func&& f) {
  return detail::Schedule<E>(e, std::forward<Func>(f));
}

/**
 * TODO(MBkkt) add description
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(Func&& f) {
  return detail::Schedule<E>(MakeInline(), std::forward<Func>(f));
}

}  // namespace yaclib

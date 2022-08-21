#pragma once

#include <yaclib/lazy/task.hpp>

namespace yaclib {

/**
 * TODO(MBkkt) add description
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(IExecutor& e, Func&& f) {
  auto task = detail::Schedule<E>(&e, std::forward<Func>(f));
  e.IncRef();
  task.GetCore()->_caller = &e;
  return task;
}

/**
 * TODO(MBkkt) add description
 */
template <typename E = StopError, typename Func>
/*Task*/ auto Schedule(Func&& f) {
  auto task = detail::Schedule<E>(nullptr, std::forward<Func>(f));
  task.GetCore()->_caller = nullptr;
  return task;
}

}  // namespace yaclib

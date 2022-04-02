#pragma once

#include <yaclib/config.hpp>
#include <yaclib/executor/detail/unique_task.hpp>
#include <yaclib/executor/executor.hpp>

#include <utility>

namespace yaclib {

/**
 * Submit given functor for details \see Submit
 *
 * This method creates ITask with one allocation and call Submit(ITask)
 * \param f functor to execute
 * \return true if the task is accepted and scheduled for execution, false if the task is rejected
 */
template <typename Functor>
bool Submit(const IExecutorPtr& executor, Functor&& f) {
  static_assert(!std::is_base_of_v<ITask, std::decay_t<Functor>>);
  auto task = detail::MakeUniqueTask(std::forward<Functor>(f));
  if (executor->Submit(*task)) {
    return true;
  }
  task->DecRef();
  return false;
}

}  // namespace yaclib

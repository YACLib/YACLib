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
 */
template <typename Functor>
void Submit(const IExecutorPtr& executor, Functor&& f) {
  static_assert(!std::is_base_of_v<ITask, std::decay_t<Functor>>);
  auto task = detail::MakeUniqueTask(std::forward<Functor>(f));
  executor->Submit(*task);
}

}  // namespace yaclib

#pragma once

#include <yaclib/executor/detail/unique_job.hpp>
#include <yaclib/executor/executor.hpp>

#include <utility>

namespace yaclib {

/**
 * Submit given functor for details \see Submit
 *
 * This method creates Job with one allocation and call Submit(Job)
 * \param f functor to execute
 */
template <typename Func>
void Submit(IExecutor& executor, Func&& f) {
  static_assert(!std::is_base_of_v<Job, std::decay_t<Func>>);
  auto task = detail::MakeUniqueJob(std::forward<Func>(f));
  executor.Submit(*task);
}

}  // namespace yaclib

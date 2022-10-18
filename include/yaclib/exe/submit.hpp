#pragma once

#include <yaclib/exe/detail/unique_job.hpp>
#include <yaclib/exe/executor.hpp>

#include <utility>

namespace yaclib {

/**
 * Submit given func for details \see Submit
 *
 * This method creates Job with one allocation and call Submit(Job)
 * \param f func to execute
 */
template <typename Func>
void Submit(IExecutor& executor, Func&& f) {
  static_assert(!std::is_base_of_v<Job, std::decay_t<Func>>, "Please use executor.Submit(job)");
  auto* job = detail::MakeUniqueJob(std::forward<Func>(f));
  executor.Submit(*job);
}

}  // namespace yaclib

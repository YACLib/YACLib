#pragma once

#include <yaclib/executor/task.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {

class IExecutor : public IRef {
 public:
  /**
   * Executor tag
   *
   * \enum Custom, Inline, Strand, ThreadPool, SingleThread
   */
  enum class Type {
    Custom = 0,
    Inline,
    Strand,
    ThreadPool,
    SingleThread,
  };

  /**
   * Return type of this executor
   */
  [[nodiscard]] virtual Type Tag() const = 0;

  /**
   * Submit given task. This method may either submit or reject the task
   *
   * This method increments reference counter if task is submitted.
   * \param task task to execute
   * \return true if the task is submitted, false if the task is rejected
   */
  virtual bool Submit(ITask& task) noexcept = 0;
};

using IExecutorPtr = IntrusivePtr<IExecutor>;

}  // namespace yaclib

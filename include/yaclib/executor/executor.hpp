#pragma once

#include <yaclib/config.hpp>
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
   * Submit given task. This method may either Call or Cancel the task
   *
   * This method increments reference counter if task is submitted.
   * \param task task to execute
   */
  virtual void Submit(ITask& task) noexcept = 0;
};

using IExecutorPtr = IntrusivePtr<IExecutor>;

}  // namespace yaclib

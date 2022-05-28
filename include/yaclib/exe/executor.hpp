#pragma once

#include <yaclib/exe/job.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {

class IExecutor : public IRef {
 public:
  /**
   * Executor tag
   *
   * \enum Custom, Inline, Strand, ThreadPool, SingleThread
   */
  enum class Type : char {
    Custom = 0,
    Inline = 1,
    Strand = 2,
    ThreadPool = 3,
    SingleThread = 4,
  };

  /**
   * Return type of this executor
   */
  [[nodiscard]] virtual Type Tag() const = 0;

  /**
   * Submit given task. This method may either Call or Drop the task
   *
   * This method increments reference counter if task is submitted.
   * \param task task to execute
   */
  virtual void Submit(Job& task) noexcept = 0;
};

using IExecutorPtr = IntrusivePtr<IExecutor>;

}  // namespace yaclib

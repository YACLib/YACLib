#pragma once

#include <yaclib/exe/job.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {

class IExecutor : public IRef {
 public:
  /**
   * Executor tag
   *
   * TODO(kononovk) add description
   * \enum
   * Custom
   * Inline
   * Manual
   * Strand
   * SingleThread
   * FairThreadPool
   * GolangThreadPool
   */
  enum class Type : unsigned char {
    Custom = 0,
    Inline = 1,
    Manual = 2,
    Strand = 3,
    SingleThread = 4,
    FairThreadPool = 5,
    GolangThreadPool = 6,
  };

  /**
   * Return type of this executor
   */
  [[nodiscard]] virtual Type Tag() const noexcept = 0;

  /**
   * Return true if executor still alive, that means job passed to submit will be Call
   */
  [[nodiscard]] virtual bool Alive() const noexcept = 0;

  /**
   * Submit given job. This method may either Call or Drop the job
   *
   * This method increments reference counter if task is submitted.
   *
   * Call if executor is Alive, otherwise Drop
   * \param job job to execute
   */
  virtual void Submit(Job& job) noexcept = 0;
};

using IExecutorPtr = IntrusivePtr<IExecutor>;

}  // namespace yaclib

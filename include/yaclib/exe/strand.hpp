#pragma once

#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>

#include <yaclib_std/atomic>

namespace yaclib {

class Strand : private Job, public IExecutor {
  // Inheritance from two IRef's, but that's okay, because they are pure virtual
 public:
  explicit Strand(IExecutorPtr e) noexcept;

  ~Strand() noexcept override;

  [[nodiscard]] Type Tag() const noexcept final;

  [[nodiscard]] bool Alive() const noexcept final;

  void Submit(Job& job) noexcept final;

 private:
  void Call() noexcept final;

  void Drop() noexcept final;

  Node* Mark() noexcept;

  IExecutorPtr _executor;
  yaclib_std::atomic<Node*> _jobs{Mark()};
};

/**
 * Strand is the asynchronous analogue of a mutex
 *
 * It guarantees that the tasks scheduled for it will be executed strictly sequentially.
 * Strand itself does not have its own threads, it decorates another executor and uses it to run its tasks.
 * \param e executor to decorate
 * \return pointer to new Strand instance
 */
IExecutorPtr MakeStrand(IExecutorPtr e);

}  // namespace yaclib

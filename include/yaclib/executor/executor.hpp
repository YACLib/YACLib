#pragma once

#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/task.hpp>

#include <memory>

namespace yaclib::executor {

class IExecutor : public IRef {
 public:
  /**
   * \brief Executor tag
   * \enum Custom, ThreadPool, Serial, Inline, SingleThread
   */
  enum class Type {
    Custom = 0,
    ThreadPool,
    Serial,
    Inline,
    SingleThread,
  };

  /**
   * \brief Return type of this executor
   */
  [[nodiscard]] virtual Type Tag() const = 0;

  /**
   * \brief Execute given functor for details \see Execute
   *
   * This method creates ITask with one allocation and call Execute(ITask)
   * \param functor task to execute
   * \return true if the task is accepted and scheduled for execution, false if the task is rejected
   */
  template <typename Functor, std::enable_if_t<!std::is_base_of_v<ITask, std::decay_t<Functor>>, int> = 0>
  bool Execute(Functor&& functor) {
    auto task = detail::MakeUniqueTask(std::forward<Functor>(functor));
    return Execute(*task);
  }

  /**
   * \brief Execute given task. This method may either accept or reject the task.
   *
   * This method always increments reference counter for the given task,
   * even if it rejects the task after that.
   * If the task is rejected, the reference counter is decremented back,
   * otherwise the counter is decremented when the task is no longer needed by the executor.
   * \param task task to execute
   * \return true if the task is accepted and scheduled for execution, false if the task is rejected
   */
  virtual bool Execute(ITask& task) = 0;
};

using IExecutorPtr = container::intrusive::Ptr<IExecutor>;

}  // namespace yaclib::executor

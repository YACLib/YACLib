#pragma once

#include <yaclib/task.hpp>

#include <memory>

namespace yaclib::executor {

class IExecutor {
 public:
  template <typename Functor>
  void Execute(Functor&& functor) {
    Execute(*detail::MakeFunctorTask(std::forward<Functor>(functor)));
  }

  virtual void Execute(ITask& task) = 0;

  virtual ~IExecutor() = default;
};

using IExecutorPtr = std::shared_ptr<IExecutor>;

}  // namespace yaclib::executor

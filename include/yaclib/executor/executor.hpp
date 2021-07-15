#pragma once

#include <yaclib/task.hpp>

#include <memory>

namespace yaclib::executor {

class IExecutor {
 public:
  virtual void Execute(ITaskPtr task) = 0;

  virtual ~IExecutor() = default;
};

using IExecutorPtr = std::shared_ptr<IExecutor>;

}  // namespace yaclib::executor

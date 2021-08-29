#pragma once

#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/task.hpp>

#include <memory>

namespace yaclib::executor {

class IExecutor : public IRef {
 public:
  template <typename Functor, std::enable_if_t<!std::is_base_of_v<ITask, std::decay_t<Functor>>, int> = 0>
  bool Execute(Functor&& functor) {
    auto task = detail::MakeUniqueTask(std::forward<Functor>(functor));
    return Execute(*task);
  }

  virtual bool Execute(ITask& task) = 0;
};

using IExecutorPtr = container::intrusive::Ptr<IExecutor>;

}  // namespace yaclib::executor

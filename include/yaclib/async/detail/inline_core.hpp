#pragma once

#include <yaclib/config.hpp>
#include <yaclib/executor/task.hpp>

namespace yaclib::detail {

class InlineCore : public ITask {
 public:
  enum class State {
    Empty = 0,
    HasResult,
    HasCallback,
    HasCallbackInline,
    HasAsyncCallback,
    HasWait,
    HasStop,
  };

  void Call() noexcept override;
  void Cancel() noexcept override;

  virtual void CallInline(InlineCore&, State) noexcept;
};

}  // namespace yaclib::detail

#pragma once

#include <yaclib/executor/job.hpp>

namespace yaclib::detail {

class InlineCore : public Job {
 public:
  enum class State : char {
    Empty = 0,
    HasResult = 1,
    HasCallback = 2,
    HasCallbackInline = 3,
    HasAsyncCallback = 4,
    HasWait = 5,
    HasStop = 6,
  };

  void Call() noexcept override;
  void Cancel() noexcept override;

  virtual void CallInline(InlineCore&, State) noexcept;
};

}  // namespace yaclib::detail

#pragma once

#include <yaclib/executor/task.hpp>

namespace yaclib::detail {

class InlineCore : public ITask {
 public:
  virtual void CallInline(InlineCore* caller) noexcept = 0;

  void Call() noexcept override;
  void Cancel() noexcept override;
};

}  // namespace yaclib::detail

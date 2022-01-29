#pragma once

#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib::detail {

class BaseCore : public InlineCore {
 public:
  enum class State {
    Empty,
    HasResult,
    HasCallback,
    HasCallbackInline,
    HasWait,
    HasStop,
  };

  explicit BaseCore(State s) noexcept;

  [[nodiscard]] State GetState() const noexcept;
  void SetState(State s) noexcept;

  [[nodiscard]] IExecutorPtr GetExecutor() const noexcept;
  void SetExecutor(IExecutorPtr executor) noexcept;

  void SetCallback(util::Ptr<BaseCore> callback);
  void SetCallbackInline(util::Ptr<InlineCore> callback);
  bool SetWait(util::IRef& callback) noexcept;
  bool ResetWait() noexcept;

 protected:
  yaclib_std::atomic<State> _state;
  util::Ptr<ITask> _caller;
  IExecutorPtr _executor{MakeInline()};
  util::Ptr<IRef> _callback;

  void Execute() noexcept;
  void ExecuteInline() noexcept;

  void Clean() noexcept;
  void Cancel() noexcept final;
};

}  // namespace yaclib::detail

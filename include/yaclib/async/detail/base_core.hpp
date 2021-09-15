#pragma once

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <atomic>

namespace yaclib::detail {

class BaseCore : public ITask {
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

  void SetCallback(util::Ptr<ITask> callback);
  void SetCallbackInline(util::Ptr<ITask> callback);
  bool SetWait(util::IRef& callback) noexcept;
  bool ResetWait() noexcept;

 protected:
  std::atomic<State> _state;
  util::Ptr<ITask> _caller;
  IExecutorPtr _executor{MakeInline()};
  util::Ptr<IRef> _callback;

  void Execute() noexcept;
  void ExecuteInline() noexcept;

  void Clean() noexcept;

  void Call() noexcept override;
  virtual void CallInline(void* caller) noexcept = 0;
  void Cancel() noexcept final;
};

}  // namespace yaclib::detail
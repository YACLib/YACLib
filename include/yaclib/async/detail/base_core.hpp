#pragma once

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <atomic>

namespace yaclib::detail {

class BaseCore : public ITask {
 protected:
  enum class State {
    Empty,
    HasResult,
    HasCallback,
    HasInlineCallback,
    HasLastInlineCallback,
    HasWaitCallback,
    Stopped,
  };
  explicit BaseCore(State state);

 public:
  [[nodiscard]] bool Ready() const noexcept;

  void Stop() noexcept;

  [[nodiscard]] IExecutorPtr GetExecutor() const noexcept;

  void SetExecutor(IExecutorPtr executor) noexcept;

  void SetCallback(util::Ptr<ITask> callback);

  void SetInlineCallback(util::Ptr<ITask> callback);

  void SetLastInlineCallback(util::Ptr<ITask> callback);

  bool SetWaitCallback(util::IRef& callback) noexcept;

  bool ResetAfterTimeout() noexcept;

 protected:
  std::atomic<State> _state;
  util::Ptr<ITask> _caller;
  IExecutorPtr _executor{MakeInline()};
  util::Ptr<IRef> _callback;

  void Execute();
  void InlineExecute();
  void LastInlineExecute();

  void Call() noexcept override;
  virtual void InlineCall(void* context);
  virtual void LastInlineCall(void* context);
  void Cancel() noexcept final;

  void Clean() noexcept;
};

}  // namespace yaclib::detail

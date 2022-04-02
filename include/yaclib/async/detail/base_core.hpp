#pragma once

#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

namespace yaclib::detail {

class BaseCore : public InlineCore {
 public:
  explicit BaseCore(State s) noexcept;

  void SetExecutor(IExecutorPtr executor) noexcept;
  [[nodiscard]] IExecutorPtr GetExecutor() const noexcept;

  void SetCallback(BaseCore& callback) noexcept;
  void SetCallbackInline(InlineCore& callback, bool async = false) noexcept;
  [[nodiscard]] bool SetWait(IRef& callback) noexcept;
  [[nodiscard]] bool ResetWait() noexcept;

  virtual void SetStop() = 0;

  void Stop() noexcept;
  [[nodiscard]] bool Empty() const noexcept;
  [[nodiscard]] bool Alive() const noexcept;

 protected:
  yaclib_std::atomic<State> _state;
  IntrusivePtr<ITask> _caller;
  IExecutorPtr _executor;
  IntrusivePtr<IRef> _callback;

  void Submit() noexcept;
  void Cancel() noexcept final;

 private:
  void Clean() noexcept;
};

}  // namespace yaclib::detail

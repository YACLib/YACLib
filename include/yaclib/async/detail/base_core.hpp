#pragma once

#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>

namespace yaclib::detail {

class BaseCore : public InlineCore {
 public:
  explicit BaseCore(State s) noexcept;

  void SetExecutor(IExecutor* executor) noexcept;
  [[nodiscard]] IExecutor* GetExecutor() const noexcept;

  void SetCallback(BaseCore& callback) noexcept;
  void SetCallbackInline(InlineCore& callback, bool async = false) noexcept;
  [[nodiscard]] bool SetWait(IRef& callback) noexcept;
  [[nodiscard]] bool ResetWait() noexcept;

  void Stop() noexcept;
  [[nodiscard]] bool Empty() const noexcept;
  [[nodiscard]] bool Alive() const noexcept;

 protected:
  yaclib_std::atomic<State> _state;
  IExecutorPtr _executor;
  IntrusivePtr<IRef> _callback;
  IntrusivePtr<IRef> _caller;

  void Submit() noexcept;
};

}  // namespace yaclib::detail

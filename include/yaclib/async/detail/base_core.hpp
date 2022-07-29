#pragma once

#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>
#if YACLIB_CORO != 0
#  include <yaclib/coroutine/coroutine.hpp>
#endif

namespace yaclib::detail {

class BaseCore : public InlineCore {
 public:
  [[nodiscard]] bool Empty() const noexcept;
  [[nodiscard]] bool Alive() const noexcept;

#if YACLIB_CORO != 0
  virtual yaclib_std::coroutine_handle<> GetHandle() noexcept {
    return yaclib_std::coroutine_handle<>{};  // plug, see coroutine/detail/promise_type.hpp
  }
#endif

  void SetCall(BaseCore& callback) noexcept;
  void SetHere(InlineCore& callback, State state) noexcept;
  void SetWait(State state) noexcept;
  [[nodiscard]] bool SetWait(IRef& callback, State state) noexcept;
  [[nodiscard]] bool ResetWait() noexcept;

  [[nodiscard]] IExecutorPtr& GetExecutor() noexcept;
  void SetExecutor(IExecutorPtr executor) noexcept;

  void SetResult() noexcept;

 protected:
  explicit BaseCore(State callback) noexcept;

#ifdef YACLIB_LOG_DEBUG
  ~BaseCore() noexcept override;
#endif

  yaclib_std::atomic_uint64_t _callback;
  IRef* _caller{nullptr};
  IExecutorPtr _executor;

 private:
  [[nodiscard]] bool SetCallback(IRef& callback, State state) noexcept;

  void Submit(BaseCore& callback) noexcept;
  void Submit(InlineCore& callback, State state) noexcept;
};

}  // namespace yaclib::detail

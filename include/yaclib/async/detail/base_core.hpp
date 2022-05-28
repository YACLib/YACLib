#pragma once

#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>

namespace yaclib::detail {

class CCore : public PCore {
 public:
  [[nodiscard]] bool Empty() const noexcept;
  [[nodiscard]] bool Alive() const noexcept;

  void SetCall(CCore& callback) noexcept;
  void SetHere(PCore& callback, State state) noexcept;
  void SetWait(State state) noexcept;
  [[nodiscard]] bool SetWait(IRef& callback, State state) noexcept;
  [[nodiscard]] bool ResetWait() noexcept;

  void SetCaller(CCore& caller) noexcept;

  [[nodiscard]] IExecutorPtr& GetExecutor() noexcept;
  void SetExecutor(IExecutorPtr executor) noexcept;

  void SetResult() noexcept;

 protected:
  explicit CCore(State callback) noexcept;

#ifdef YACLIB_LOG_DEBUG
  ~CCore() noexcept override;
#endif

  yaclib_std::atomic<std::uintptr_t> _callback;
  IRef* _caller{nullptr};
  IExecutorPtr _executor;

 private:
  [[nodiscard]] bool SetCallback(IRef& callback, State state) noexcept;

  void Submit(CCore& callback) noexcept;
  void Submit(PCore& callback, State state) noexcept;
};

}  // namespace yaclib::detail

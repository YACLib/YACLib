#pragma once

// #include <yaclib/async/detail/result_core.hpp>
#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/thread.hpp>

namespace yaclib::detail {

class ViaAwaiter : public ITask {
 public:
  explicit ViaAwaiter(yaclib::IExecutorPtr e);

  void IncRef() noexcept final;
  void DecRef() noexcept final;

  void Call() noexcept final;

  void Cancel() noexcept final;
  bool await_ready();
  void await_resume();
  template <typename V, typename E>
  bool await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) {
    _handle = handle;
    _promise = &handle.promise();
    return _executor->Submit(*this);
  }

 private:
  enum class State : char { Empty = 0, HasCalled, HasCancelled };

  IExecutorPtr _executor = nullptr;
  yaclib_std::coroutine_handle<> _handle;
  BaseCore* _promise;
  State _state;
};
}  // namespace yaclib::detail

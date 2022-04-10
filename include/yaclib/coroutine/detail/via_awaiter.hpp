#pragma once

#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/thread.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib::detail {

class ViaAwaiter : public ITask {
 public:
  explicit ViaAwaiter(IExecutorPtr e);

  bool await_ready();
  void await_resume();
  template <typename V, typename E>
  bool await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) {
    _handle = handle;
    _promise = &handle.promise();
    _executor->Submit(*this);
    return true;  // if coroutine was not submitted, then it is kind of invalid
  }

 private:
  void IncRef() noexcept final;
  void DecRef() noexcept final;

  void Call() noexcept final;

  void Cancel() noexcept final;

 private:
  enum class State : char { Empty = 0, HasCalled, HasCancelled };

  IExecutorPtr _executor;
  yaclib_std::coroutine_handle<> _handle;
  BaseCore* _promise;
  State _state;
};
}  // namespace yaclib::detail

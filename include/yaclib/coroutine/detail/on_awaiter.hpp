#pragma once

#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/job.hpp>

namespace yaclib::detail {

class OnAwaiter {
 public:
  explicit OnAwaiter(IExecutor& e);

  static bool await_ready() noexcept {
    return false;
  }

  template <typename V, typename E>
  void await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) noexcept {
    _executor.Submit(handle.promise());
  }

  static void await_resume() noexcept {
  }

 private:
  IExecutor& _executor;
};

}  // namespace yaclib::detail

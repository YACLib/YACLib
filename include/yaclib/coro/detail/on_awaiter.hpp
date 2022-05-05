#pragma once

#include <yaclib/coro/coroutine.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>

namespace yaclib::detail {

class OnAwaiter {
 public:
  explicit OnAwaiter(IExecutor& e);

  YACLIB_INLINE bool await_ready() const noexcept {
    return false;
  }

  template <typename V, typename E>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) const noexcept {
    _executor.Submit(handle.promise());
  }

  YACLIB_INLINE void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
};

}  // namespace yaclib::detail

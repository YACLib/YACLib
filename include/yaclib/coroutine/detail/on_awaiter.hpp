#pragma once

#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/job.hpp>

namespace yaclib::detail {

class OnAwaiter final {
 public:
  explicit OnAwaiter(IExecutor& e);

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    _executor.Submit(handle.promise());
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
};

}  // namespace yaclib::detail

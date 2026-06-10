#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>

namespace yaclib::detail {

// E defaults to IExecutor (the type-erased path); when E is a concrete final
// executor the Submit call below devirtualizes.
template <typename E = IExecutor>
class [[nodiscard]] OnAwaiter final {
 public:
  YACLIB_INLINE explicit OnAwaiter(E& e) noexcept : _executor{e} {
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    auto& promise = handle.promise();
    promise._executor = &_executor;
    _executor.Submit(promise);
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  E& _executor;
};

}  // namespace yaclib::detail

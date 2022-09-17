#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {
namespace detail {

class [[nodiscard]] Yield final {
 public:
  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    auto& promise = handle.promise();
    YACLIB_ASSERT(promise._executor != nullptr);
    promise._executor->Submit(promise);
  }

  constexpr void await_resume() const noexcept {
  }
};

}  // namespace detail

/**
 * Reschedule current job to it executor
 * Useful for timeout checks, or if you job very big and doing only cpu not suspend work
 */
inline constexpr detail::Yield kYield;

}  // namespace yaclib

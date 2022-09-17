#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {
namespace detail {

template <bool Yield>
class [[nodiscard]] CurrentAwaiter final {
 public:
  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& promise = handle.promise();
    _executor = promise._executor.Get();
    YACLIB_ASSERT(_executor != nullptr);
    if constexpr (Yield) {
      _executor->Submit(promise);
    } else {
      return false;
    }
  }

  [[nodiscard]] YACLIB_INLINE IExecutor& await_resume() const noexcept {
    return *_executor;
  }

 private:
  IExecutor* _executor = nullptr;
};

}  // namespace detail

/**
 * Get current executor
 */
YACLIB_INLINE detail::CurrentAwaiter<false> CurrentExecutor() noexcept {
  return {};
}

/**
 * Instead of
 * \code
 * co_await yaclib::kYield;
 * auto& executor = co_await yaclib::Current();
 * \endcode
 * or
 * \code
 * auto& executor = co_await yaclib::kCurrent;
 * co_await On(executor);
 * \endcode
 * you can write
 * auto& executor = co_await yaclib::Yield();
 */
YACLIB_INLINE detail::CurrentAwaiter<true> Yield() noexcept {
  return {};
}

}  // namespace yaclib

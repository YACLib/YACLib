#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

class AwaitAwaiter final {
 public:
  template <typename... Cores>
  explicit AwaitAwaiter(Cores&... cores);

  template <typename It>
  explicit AwaitAwaiter(It it, std::size_t count);

  YACLIB_INLINE bool await_ready() const noexcept {
    return _await_core.GetRef() == 1;
  }

  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<> handle) noexcept {
    _await_core.handle = std::move(handle);
    return !_await_core.SubEqual(1);
  }

  YACLIB_INLINE void await_resume() const noexcept {
  }

 private:
  struct Handle : IRef {
    yaclib_std::coroutine_handle<> handle;
  };

  struct HandleDeleter final {
    static void Delete(Handle& handle) noexcept {
      YACLIB_DEBUG(!handle.handle, "saved to resume handle is null");
      YACLIB_DEBUG(handle.handle.done(), "handle for resume is done");
      handle.handle.resume();  // TODO(mkornaukhov03) resume on custom IExecutor
    }
  };

  AtomicCounter<Handle, HandleDeleter> _await_core;
};

template <typename... Cores>
AwaitAwaiter::AwaitAwaiter(Cores&... cores) : _await_core{sizeof...(cores) + 1} {
  static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<BaseCore, Cores>), "Futures must be Future in Wait function");
  const auto wait_count = (... + static_cast<std::size_t>(cores.SetWait(_await_core, InlineCore::kWaitNope)));
  _await_core.count.fetch_sub(sizeof...(cores) - wait_count, std::memory_order_relaxed);
}

template <typename It>
AwaitAwaiter::AwaitAwaiter(It it, std::size_t count) : _await_core{count + 1} {
  std::size_t wait_count = 0;
  for (std::size_t i = 0; i != count; ++i) {
    wait_count += static_cast<std::size_t>(it->GetCore()->SetWait(_await_core, InlineCore::kWaitNope));
    ++it;
  }
  _await_core.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

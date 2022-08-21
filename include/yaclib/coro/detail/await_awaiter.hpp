#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coroutine.hpp>
#include <yaclib/exe/thread_pool.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

class [[nodiscard]] AwaitAwaiter final {
 public:
  template <typename... Cores>
  explicit AwaitAwaiter(Cores&... cores) noexcept;

  template <typename It>
  explicit AwaitAwaiter(It it, std::size_t count) noexcept;

  YACLIB_INLINE bool await_ready() const noexcept {
    return _event.GetRef() == 1;
  }
  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _event.job = &handle.promise();
    return !_event.SubEqual(1);
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  struct Event : IRef {
    YACLIB_INLINE explicit Event(IExecutor& e) noexcept : executor{e} {
    }

    YACLIB_INLINE void Set() noexcept {
      YACLIB_ASSERT(job);
      executor.Submit(*job);
    }

    IExecutor& executor;
    Job* job{nullptr};
  };

  AtomicCounter<Event, SetDeleter> _event;
};

template <typename... Cores>
AwaitAwaiter::AwaitAwaiter(Cores&... cores) noexcept : _event{sizeof...(cores) + 1, CurrentThreadPool()} {
  static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<BaseCore, Cores>), "Futures must be Future in Wait function");
  const auto wait_count = (... + static_cast<std::size_t>(cores.SetWait(_event, InlineCore::kWaitNope)));
  _event.count.fetch_sub(sizeof...(cores) - wait_count, std::memory_order_relaxed);
}

template <typename It>
AwaitAwaiter::AwaitAwaiter(It it, std::size_t count) noexcept : _event{count + 1, CurrentThreadPool()} {
  std::size_t wait_count = 0;
  for (std::size_t i = 0; i != count; ++i) {
    wait_count += static_cast<std::size_t>(it->GetCore()->SetWait(_event, InlineCore::kWaitNope));
    ++it;
  }
  _event.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/lazy/task.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <bool Single>
struct [[nodiscard]] AwaitAwaiter {
  AwaitAwaiter(BaseCore& caller) noexcept : _caller{caller} {
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return !_caller.Empty();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _caller.SetCallback(handle.promise(), BaseCore::kInline);
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  BaseCore& _caller;
};

class AwaitEvent final : public InlineCore, public AtomicCounter<NopeBase, NopeDeleter> {
 public:
  using AtomicCounter<NopeBase, NopeDeleter>::AtomicCounter;

 private:
  void Here(BaseCore& caller) noexcept final {
    if (this->SubEqual(1)) {
      return static_cast<InlineCore*>(next)->Here(caller);
    }
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    if (this->SubEqual(1)) {
      return static_cast<InlineCore*>(next)->Next(caller);
    }
    return yaclib_std::noop_coroutine();
  }
#endif
};

template <>
class [[nodiscard]] AwaitAwaiter<false> final {
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
    _event.next = &handle.promise();
    return !_event.SubEqual(1);
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  AwaitEvent _event;
};

template <typename... Cores>
AwaitAwaiter<false>::AwaitAwaiter(Cores&... cores) noexcept : _event{sizeof...(cores) + 1} {
  static_assert(sizeof...(cores) >= 2, "Number of futures must be at least two");
  static_assert((... && std::is_same_v<BaseCore, Cores>), "Futures must be Future in Wait function");
  const auto wait_count = (... + static_cast<std::size_t>(cores.SetCallback(_event, BaseCore::kInline)));
  _event.count.fetch_sub(sizeof...(cores) - wait_count, std::memory_order_relaxed);
}

template <typename It>
AwaitAwaiter<false>::AwaitAwaiter(It it, std::size_t count) noexcept : _event{count + 1} {
  std::size_t wait_count = 0;
  for (std::size_t i = 0; i != count; ++i) {
    wait_count += static_cast<std::size_t>(it->GetCore()->SetCallback(_event, BaseCore::kInline));
    ++it;
  }
  _event.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

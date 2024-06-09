#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/lazy/task.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

template <bool Single>
using AwaitOnCounterT =
  std::conditional_t<Single, OneCounter<NopeBase, NopeDeleter>, AtomicCounter<NopeBase, NopeDeleter>>;

template <bool Single>
class AwaitOnEvent final : public InlineCore, public AwaitOnCounterT<Single> {
 public:
  explicit AwaitOnEvent(std::size_t n) noexcept : AwaitOnCounterT<Single>{n} {
  }

  BaseCore* job{nullptr};

 private:
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& /*caller*/) noexcept {
    if constexpr (Single) {
      job->_executor->Submit(*job);
    } else {
      if (this->SubEqual(1)) {
        YACLIB_ASSERT(job != nullptr);
        job->_executor->Submit(*job);
      }
    }
    return Noop<SymmetricTransfer>();
  }
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif
};

template <bool Single>
struct [[nodiscard]] AwaitOnAwaiter final {
  explicit AwaitOnAwaiter(IExecutor& e, BaseCore& caller) noexcept : _executor{e}, _event{1} {
    _event.job = &caller;
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& core = handle.promise();
    core._executor = &_executor;
    auto* caller = _event.job;
    _event.job = &core;

    if (!caller->SetCallback(_event)) {
      _executor.Submit(core);
    }
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
  AwaitOnEvent<true> _event;
};

template <>
class [[nodiscard]] AwaitOnAwaiter<false> final {
 public:
  template <typename... Cores>
  explicit AwaitOnAwaiter(IExecutor& e, Cores&... cores) noexcept;

  template <typename It>
  explicit AwaitOnAwaiter(IExecutor& e, It it, std::size_t count) noexcept;

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& core = handle.promise();
    core._executor = &_executor;
    _event.job = &core;

    if (_event.SubEqual(1)) {
      _executor.Submit(core);
    }
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
  AwaitOnEvent<false> _event;
};

template <typename... Cores>
AwaitOnAwaiter<false>::AwaitOnAwaiter(IExecutor& e, Cores&... cores) noexcept
  : _executor{e}, _event{sizeof...(cores) + 1} {
  static_assert(sizeof...(cores) >= 2, "Number of futures must be at least two");
  static_assert((... && std::is_same_v<BaseCore, Cores>), "Futures must be Future in Wait function");
  const auto wait_count = (... + static_cast<std::size_t>(cores.SetCallback(_event)));
  _event.count.fetch_sub(sizeof...(cores) - wait_count, std::memory_order_relaxed);
}

template <typename It>
AwaitOnAwaiter<false>::AwaitOnAwaiter(IExecutor& e, It it, std::size_t count) noexcept
  : _executor{e}, _event{count + 1} {
  std::size_t wait_count = 0;
  for (std::size_t i = 0; i != count; ++i) {
    wait_count += static_cast<std::size_t>(it->GetCore()->SetCallback(_event));
    ++it;
  }
  _event.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

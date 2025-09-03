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
class AwaitOnEvent : public InlineCore, public AwaitOnCounterT<Single> {
 public:
  static constexpr auto kShared = false;

  AwaitOnEvent& GetCall() noexcept {
    return *this;
  }

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

 public:
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif
};

template <typename Handle>
struct [[nodiscard]] AwaitOnAwaiter final {
  explicit AwaitOnAwaiter(IExecutor& e, Handle caller) noexcept : _executor{e}, _event{1} {
    _event.job = &caller.core;
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& core = handle.promise();
    core._executor = &_executor;
    Handle caller_handle{*_event.job};
    _event.job = &core;

    if (!caller_handle.SetCallback(_event)) {
      _executor.Submit(core);
    }
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
  AwaitOnEvent<true> _event;
};

template <typename Event>
class [[nodiscard]] MultiAwaitOnAwaiter final {
 public:
  static constexpr auto kShared = Event::kShared;

  template <typename... Handles>
  explicit MultiAwaitOnAwaiter(IExecutor& e, Handles... handles) noexcept
    : _executor{e}, _event{sizeof...(handles) + 1} {
    static_assert(sizeof...(handles) >= 2, "Number of futures must be at least two");

    const auto wait_count = [&] {
      if constexpr (!kShared) {
        auto setter = [&](auto handle) {
          return handle.SetCallback(_event);
        };
        return (... + static_cast<std::size_t>(setter(handles)));
      } else {
        auto setter = [&, callback_count = std::size_t{}](auto handle) mutable {
          if constexpr (std::is_same_v<decltype(handle), UniqueHandle>) {
            return handle.SetCallback(_event);
          } else {
            return handle.SetCallback(_event.callbacks[callback_count++]);
          }
        };
        return (... + static_cast<std::size_t>(setter(handles)));
      }
    }();

    _event.count.fetch_sub(sizeof...(handles) - wait_count, std::memory_order_relaxed);
  }

  template <typename It>
  explicit MultiAwaitOnAwaiter(IExecutor& e, It it, std::size_t count) noexcept : _executor{e}, _event{count + 1} {
    std::size_t wait_count = 0;
    for (std::size_t i = 0; i != count; ++i) {
      YACLIB_ASSERT(it->Valid());
      if constexpr (std::is_same_v<decltype(it->GetHandle()), UniqueHandle>) {
        wait_count += static_cast<std::size_t>(it->GetHandle().SetCallback(_event));
      } else {
        wait_count += static_cast<std::size_t>(it->GetHandle().SetCallback(_event.callbacks[i]));
      }
      ++it;
    }
    _event.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
  }

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
  Event _event;
};

}  // namespace yaclib::detail

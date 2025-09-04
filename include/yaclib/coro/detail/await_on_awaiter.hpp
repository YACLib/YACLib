#pragma once

#include <yaclib/algo/detail/shared_event.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/exe/executor.hpp>
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

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif

 protected:
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
};

template <typename Handle>
struct [[nodiscard]] AwaitOnAwaiter final : AwaitOnEvent<false> {
  explicit AwaitOnAwaiter(IExecutor& e, Handle caller) noexcept : AwaitOnEvent<false>{1}, _executor{e} {
    job = &caller.core;
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& core = handle.promise();
    core._executor = &_executor;
    Handle caller_handle{*job};
    job = &core;

    if (!caller_handle.SetCallback(*this)) {
      _executor.Submit(core);
    }
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
};

template <typename Event>
class [[nodiscard]] MultiAwaitOnAwaiter final : public Event {
 public:
  static constexpr auto kShared = Event::kShared;

  template <typename... Handles>
  explicit MultiAwaitOnAwaiter(IExecutor& e, Handles... handles) noexcept
    : Event{sizeof...(handles) + 1}, _executor{e} {
    SetCallbacksStatic(*this, handles...);
  }

  template <typename It>
  explicit MultiAwaitOnAwaiter(IExecutor& e, It it, std::size_t count) noexcept : Event{count + 1}, _executor{e} {
    SetCallbacksDynamic(*this, it, count);
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    auto& core = handle.promise();
    core._executor = &_executor;
    this->job = &core;

    if (this->SubEqual(1)) {
      _executor.Submit(core);
    }
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  IExecutor& _executor;
};

}  // namespace yaclib::detail

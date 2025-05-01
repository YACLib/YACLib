#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/lazy/task.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

struct [[nodiscard]] TransferAwaiter final {
  explicit TransferAwaiter(BaseCore& caller) noexcept : _caller{caller} {
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _caller.StoreCallback(handle.promise());
    auto* next = MoveToCaller(&_caller);
#if YACLIB_SYMMETRIC_TRANSFER != 0
    return next->Next(handle.promise());
#else
    return Loop(&handle.promise(), next);
#endif
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  BaseCore& _caller;
};

template <typename V, typename E>
struct [[nodiscard]] TransferSingleAwaiter final {
  explicit TransferSingleAwaiter(ResultCorePtr<V, E>&& result) noexcept : _result{std::move(result)} {
    YACLIB_ASSERT(_result != nullptr);
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _result->StoreCallback(handle.promise());
    auto* next = MoveToCaller(_result.Get());
#if YACLIB_SYMMETRIC_TRANSFER != 0
    return next->Next(handle.promise());
#else
    return Loop(&handle.promise(), next);
#endif
  }

  auto await_resume() {
    return std::move(_result->Get()).Ok();
  }

 private:
  ResultCorePtr<V, E> _result;
};

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <bool Single>
struct [[nodiscard]] AwaitAwaiter final {
  explicit AwaitAwaiter(BaseCore& caller) noexcept : _caller{caller} {
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return !_caller.Empty();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _caller.SetCallback(handle.promise());
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
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if (this->SubEqual(1)) {
      auto* curr = static_cast<InlineCore*>(next);
      if constexpr (SymmetricTransfer) {
        return Step<true>(caller, *curr);
      } else {
        curr = curr->Here(caller);
        YACLIB_ASSERT(curr == nullptr);
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
  const auto wait_count = (... + static_cast<std::size_t>(cores.SetCallback(_event)));
  _event.count.fetch_sub(sizeof...(cores) - wait_count, std::memory_order_relaxed);
}

template <typename It>
AwaitAwaiter<false>::AwaitAwaiter(It it, std::size_t count) noexcept : _event{count + 1} {
  std::size_t wait_count = 0;
  for (std::size_t i = 0; i != count; ++i) {
    YACLIB_ASSERT(it->Valid());
    wait_count += static_cast<std::size_t>(it->GetCore()->SetCallback(_event));
    ++it;
  }
  _event.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
}

template <typename V, typename E>
class [[nodiscard]] AwaitSingleAwaiter final {
 public:
  explicit AwaitSingleAwaiter(ResultCorePtr<V, E>&& result) noexcept : _result{std::move(result)} {
    YACLIB_ASSERT(_result != nullptr);
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return !_result->Empty();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _result->SetCallback(handle.promise());
  }

  auto await_resume() {
    return std::move(_result->Get()).Ok();
  }

 private:
  ResultCorePtr<V, E> _result;
};

}  // namespace yaclib::detail

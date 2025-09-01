#pragma once

#include <yaclib/algo/detail/shared_event.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/lazy/task.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

struct [[nodiscard]] TransferAwaiter final {
  explicit TransferAwaiter(BaseCore& caller) noexcept : _caller{caller} {
    YACLIB_ASSERT(caller.Empty());
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _caller.StoreCallback(handle.promise());
    auto* next = MoveToCaller(&_caller.core);
#if YACLIB_SYMMETRIC_TRANSFER != 0
    return next->Next(handle.promise());
#else
    return Loop(&handle.promise(), next);
#endif
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  UniqueHandle _caller;
};

template <typename V, typename E>
struct [[nodiscard]] TransferSingleAwaiter final {
  explicit TransferSingleAwaiter(UniqueCorePtr<V, E>&& result) noexcept : _result{std::move(result)} {
    YACLIB_ASSERT(_result != nullptr);
    YACLIB_ASSERT(_result->Empty());
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
  UniqueCorePtr<V, E> _result;
};

template <typename Handle>
struct [[nodiscard]] AwaitAwaiter final {
  explicit AwaitAwaiter(Handle caller) noexcept : _caller{caller} {
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return !_caller.core.Empty();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    return _caller.SetCallback(handle.promise());
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  Handle _caller;
};

class AwaitEvent : public InlineCore, public AtomicCounter<NopeBase, NopeDeleter> {
 public:
  using AtomicCounter<NopeBase, NopeDeleter>::AtomicCounter;

  static constexpr auto kShared = false;

  AwaitEvent& GetCall() noexcept {
    return *this;
  }

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

template <typename Event>
class MultiAwaitAwaiter final : public Event {
 public:
  static constexpr auto kShared = Event::kShared;

  YACLIB_INLINE bool await_ready() const noexcept {
    return this->Get(std::memory_order_acquire) == 1;
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    this->next = &handle.promise();
    return !this->SubEqual(1);
  }

  constexpr void await_resume() const noexcept {
  }

  template <typename... Handles>
  explicit MultiAwaitAwaiter(Handles... handles) noexcept : Event{sizeof...(handles) + 1} {
    const auto wait_count = [&] {
      if constexpr (!kShared) {
        return (... + static_cast<std::size_t>([&](auto handle) {
                  return handle.SetCallback(*this);
                }(handles)));
      } else {
        size_t shared_count = 0;
        return (... + static_cast<std::size_t>([&](auto handle) mutable {
                  if constexpr (std::is_same_v<decltype(handle), UniqueHandle>) {
                    return handle.SetCallback(*this);
                  } else {
                    return handle.SetCallback(this->callbacks[shared_count++]);
                  }
                }(handles)));
      }
    }();
    this->count.fetch_sub(sizeof...(handles) - wait_count, std::memory_order_relaxed);
  }

  template <typename It>
  explicit MultiAwaitAwaiter(It it, std::size_t count) noexcept : Event{count + 1} {
    std::size_t wait_count = 0;
    for (std::size_t i = 0; i != count; ++i) {
      YACLIB_ASSERT(it->Valid());
      if constexpr (std::is_same_v<decltype(it->GetHandle()), UniqueHandle>) {
        wait_count += static_cast<std::size_t>(it->GetHandle().SetCallback(*this));
      } else {
        wait_count += static_cast<std::size_t>(it->GetHandle().SetCallback(this->callbacks[i]));
      }
      ++it;
    }
    this->count.fetch_sub(count - wait_count, std::memory_order_relaxed);
  }
};

template <bool Shared, typename V, typename E>
class AwaitSingleAwaiter;

template <typename V, typename E>
class [[nodiscard]] AwaitSingleAwaiter<false, V, E> final {
 public:
  explicit AwaitSingleAwaiter(UniqueCorePtr<V, E>&& result) noexcept : _result{std::move(result)} {
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
  UniqueCorePtr<V, E> _result;
};

// TODO(ocelaiwo): different overloads for lvalue and rvalue
template <typename V, typename E>
class [[nodiscard]] AwaitSingleAwaiter<true, V, E> final {
 public:
  explicit AwaitSingleAwaiter(SharedCorePtr<V, E> result) noexcept : _result{std::move(result)} {
    YACLIB_ASSERT(_result != nullptr);
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return !_result->Empty();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    return _result->SetCallback(handle.promise());
  }

  auto await_resume() const {
    return std::as_const(_result->Get()).Ok();
  }

 private:
  SharedCorePtr<V, E> _result;
};

}  // namespace yaclib::detail

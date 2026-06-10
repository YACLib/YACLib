#pragma once

#include <yaclib/algo/detail/shared_core.hpp>
#include <yaclib/algo/detail/unique_core.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/util/cast.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <exception>

namespace yaclib::detail {

template <typename V, typename T, bool Lazy, bool Shared>
class PromiseType;

struct Destroy final {
  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    auto& promise = handle.promise();
#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
    return promise.template SetResult<true>();
#elif YACLIB_SYMMETRIC_TRANSFER != 0
    return promise.template SetResult<true>().resume();
#else
    return Loop(&promise, promise.template SetResult<false>());
#endif
  }

  constexpr void await_resume() const noexcept {
  }
};

template <bool Lazy, bool Shared>
struct PromiseTypeDeleter final {
  template <typename V, typename T>
  static void Delete(ResultCore<V, T>& core) noexcept;
};

template <typename V, typename T, bool Lazy, bool Shared>
using PromiseTypeBase = std::conditional_t<Shared, AtomicCounter<SharedCore<V, T>, PromiseTypeDeleter<Lazy, Shared>>,
                                           OneCounter<UniqueCore<V, T>, PromiseTypeDeleter<Lazy, Shared>>>;

template <typename V, typename T, bool Lazy, bool Shared>
class PromiseType final : public PromiseTypeBase<V, T, Lazy, Shared> {
  using Base = PromiseTypeBase<V, T, Lazy, Shared>;
  static_assert(!Lazy || !Shared, "Not supported");

 public:
  PromiseType() noexcept : Base{Shared ? detail::kSharedRefWithFuture : 0} {
  }  // get_return_object is gonna be invoked right after ctor

  auto get_return_object() noexcept {
    if constexpr (Shared) {
      return SharedFuture<V, T>{SharedCorePtr<V, T>{NoRefTag{}, this}};
    } else if constexpr (Lazy) {
      return Task<V, T>{UniqueCorePtr<V, T>{NoRefTag{}, this}};
    } else {
      return Future<V, T>{UniqueCorePtr<V, T>{NoRefTag{}, this}};
    }
  }

  auto initial_suspend() noexcept {
    if constexpr (Lazy) {
      return yaclib_std::suspend_always{};
    } else {
      return yaclib_std::suspend_never{};
    }
  }

  Destroy final_suspend() noexcept {
    return {};
  }

  void unhandled_exception() noexcept {
    this->Store(std::current_exception());
  }

  template <typename Value>
  void return_value(Value&& value) {
    this->Store(std::forward<Value>(value));
  }

  void return_value(Unit) noexcept {
    this->Store(Unit{});
  }

  [[nodiscard]] auto Handle() noexcept {
    auto handle = yaclib_std::coroutine_handle<PromiseType>::from_promise(*this);
    YACLIB_ASSERT(handle);
    return handle;
  }

 private:
  void IncRef() noexcept final {
    return this->Add(1);
  }
  std::size_t GetRef() noexcept final {
    return this->Get();
  }
  void DecRef() noexcept final {
    this->Sub(1);
  }

  void Call() noexcept final {
    auto next = Curr();
    next.resume();
  }

  void Drop() noexcept final {
    this->Store(StopTag{});
#if YACLIB_SYMMETRIC_TRANSFER != 0
    this->template SetResult<true>().resume();
#else
    Loop(this, this->template SetResult<false>());
#endif
  }

  YACLIB_INLINE void Impl(InlineCore& caller) noexcept {
    this->_executor = std::move(DownCast<BaseCore>(caller)._executor);
    YACLIB_ASSERT(this->_executor != nullptr);
  }
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    Impl(caller);
    Call();
    return nullptr;
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    Impl(caller);
    return Curr();
  }
#endif

  [[nodiscard]] yaclib_std::coroutine_handle<> Curr() noexcept final {
    auto handle = Handle();
    YACLIB_ASSERT(!handle.done());
    return handle;
  }
};

template <bool Lazy, bool Shared>
template <typename V, typename T>
void PromiseTypeDeleter<Lazy, Shared>::Delete(ResultCore<V, T>& core) noexcept {
  auto& promise = DownCast<PromiseType<V, T, Lazy, Shared>>(core);
  auto handle = promise.Handle();
  handle.destroy();
}

}  // namespace yaclib::detail

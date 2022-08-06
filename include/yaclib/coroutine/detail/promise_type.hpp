#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <exception>

namespace yaclib::detail {

template <typename V, typename E>
class PromiseType;

struct Destroy final {
  constexpr bool await_ready() const noexcept {
    return false;
  }

  template <typename Promise>
  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    handle.promise().SetResult();
  }

  constexpr void await_resume() const noexcept {
  }
};

struct PromiseTypeDeleter final {
  template <typename V, typename E>
  static void Delete(ResultCore<V, E>& core) noexcept {
    auto& promise = static_cast<PromiseType<V, E>&>(core);
    auto handle = yaclib_std::coroutine_handle<PromiseType<V, E>>::from_promise(promise);
    YACLIB_DEBUG(!handle, "handle from promise is null");
    handle.destroy();
  }
};

template <typename V, typename E>
class BasePromiseType : public UniqueCounter<ResultCore<V, E>, PromiseTypeDeleter> {
  using Base = UniqueCounter<ResultCore<V, E>, PromiseTypeDeleter>;

 public:
  BasePromiseType() noexcept = default;  // get_return_object is gonna be invoked right after ctor

  Future<V, E> get_return_object() noexcept {
    return {ResultCorePtr<V, E>{NoRefTag{}, this}};
  }

  yaclib_std::suspend_never initial_suspend() noexcept {
    return {};
  }

  Destroy final_suspend() noexcept {
    return {};
  }

  void unhandled_exception() noexcept {
    this->Store(std::current_exception());
  }

  yaclib_std::coroutine_handle<> GetHandle() noexcept final {
    return yaclib_std::coroutine_handle<PromiseType<V, E>>::from_promise(static_cast<PromiseType<V, E>&>(*this));
  }

  void return_value(std::exception_ptr exception) noexcept {
    this->Store(std::move(exception));
  }

  void return_value(E error) noexcept {
    this->Store(std::move(error));
  }

  void return_value(StopTag) noexcept {
    this->Store(StopTag{});
  }

  void return_value(FutureBase<V, E>&& future) noexcept {
    future.GetCore().Release()->SetHere(*this, InlineCore::kHereWrap);
  }

 private:
  void Call() noexcept final {
    auto handle = yaclib_std::coroutine_handle<PromiseType<V, E>>::from_promise(static_cast<PromiseType<V, E>&>(*this));
    YACLIB_DEBUG(!handle, "handle from promise is null");
    YACLIB_DEBUG(handle.done(), "handle for resume is done");
    handle.resume();
  }

  void Here(InlineCore& caller, [[maybe_unused]] InlineCore::State state) noexcept final {
    YACLIB_ASSERT(state == InlineCore::kHereCall);
    if (!this->Alive()) {
      return this->Store(StopTag{});
    }
    auto& core = static_cast<ResultCore<V, E>&>(caller);
    this->Store(std::move(core.Get()));
  }
};

template <typename V, typename E>
class PromiseType final : public BasePromiseType<V, E> {
 public:
  void return_value(const V& value) noexcept(std::is_nothrow_copy_constructible_v<V>) {
    this->Store(value);
  }

  void return_value(V&& value) noexcept(std::is_nothrow_move_constructible_v<V>) {
    this->Store(std::move(value));
  }
};

template <typename E>
class PromiseType<void, E> final : public BasePromiseType<void, E> {
 public:
  void return_value(Unit) noexcept {
    this->Store(Unit{});
  }
};

}  // namespace yaclib::detail

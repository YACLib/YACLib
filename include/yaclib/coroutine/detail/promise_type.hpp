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
class PromiseType : public UniqueCounter<ResultCore<V, E>, PromiseTypeDeleter> {
  using Base = UniqueCounter<ResultCore<V, E>, PromiseTypeDeleter>;

 public:
  PromiseType() noexcept = default;  // get_return_object is gonna be invoked right after ctor

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

  template <typename Value>
  void return_value(Value&& value) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Value&&>) {
    this->Store(std::forward<Value>(value));
  }

  void return_value(Unit) noexcept {
    static_assert(std::is_void_v<V>);
    this->Store(Unit{});
  }

 private:
  void Call() noexcept final {
    auto handle = yaclib_std::coroutine_handle<PromiseType<V, E>>::from_promise(static_cast<PromiseType<V, E>&>(*this));
    YACLIB_DEBUG(!handle, "handle from promise is null");
    YACLIB_DEBUG(handle.done(), "handle for resume is done");
    handle.resume();
  }
};

}  // namespace yaclib::detail

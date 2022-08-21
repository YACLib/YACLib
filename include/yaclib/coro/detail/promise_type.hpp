#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/coro/coroutine.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <exception>

namespace yaclib::detail {

template <typename V, typename E, bool Lazy>
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

template <bool Lazy>
struct PromiseTypeDeleter final {
  template <typename V, typename E>
  static void Delete(ResultCore<V, E>& core) noexcept {
    auto& promise = static_cast<PromiseType<V, E, Lazy>&>(core);
    auto handle = yaclib_std::coroutine_handle<PromiseType<V, E, Lazy>>::from_promise(promise);
    YACLIB_ASSERT(handle == core.GetHandle());
    YACLIB_ASSERT(handle);
    handle.destroy();
  }
};

template <typename V, typename E, bool Lazy>
class PromiseType : public UniqueCounter<ResultCore<V, E>, PromiseTypeDeleter<Lazy>> {
  using Base = UniqueCounter<ResultCore<V, E>, PromiseTypeDeleter<Lazy>>;

 public:
  PromiseType() noexcept = default;  // get_return_object is gonna be invoked right after ctor

  auto get_return_object() noexcept {
    if constexpr (Lazy) {
      this->_caller = nullptr;
      return Task<V, E>{ResultCorePtr<V, E>{NoRefTag{}, this}};
    } else {
      return Future<V, E>{ResultCorePtr<V, E>{NoRefTag{}, this}};
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

  yaclib_std::coroutine_handle<> GetHandle() noexcept final {
    return yaclib_std::coroutine_handle<PromiseType>::from_promise(*this);
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
    auto handle = GetHandle();
    YACLIB_ASSERT(handle);
    YACLIB_ASSERT(!handle.done());
    handle.resume();
  }
};

}  // namespace yaclib::detail

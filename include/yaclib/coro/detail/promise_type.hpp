#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/coro/coro.hpp>
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
  YACLIB_INLINE auto await_suspend(yaclib_std::coroutine_handle<Promise> handle) const noexcept {
    return handle.promise().template SetResult<YACLIB_FINAL_SUSPEND_TRANSFER != 0>();
  }

  constexpr void await_resume() const noexcept {
  }
};

template <bool Lazy>
struct PromiseTypeDeleter final {
  template <typename V, typename E>
  static void Delete(ResultCore<V, E>& core) noexcept;
};

template <typename V, typename E, bool Lazy>
class PromiseType final : public OneCounter<ResultCore<V, E>, PromiseTypeDeleter<Lazy>> {
  using Base = OneCounter<ResultCore<V, E>, PromiseTypeDeleter<Lazy>>;

 public:
  PromiseType() noexcept : Base{0} {
  }  // get_return_object is gonna be invoked right after ctor

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

  template <typename Value>
  void return_value(Value&& value) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Value&&>) {
    this->Store(std::forward<Value>(value));
  }

  void return_value(Unit) noexcept {
    static_assert(std::is_void_v<V>);
    this->Store(Unit{});
  }

  auto Handle() noexcept {
    auto handle = yaclib_std::coroutine_handle<PromiseType>::from_promise(*this);
    YACLIB_ASSERT(handle);
    return handle;
  }

 private:
  void DecRef() noexcept final {
    this->Sub(1);
  }

  void Call() noexcept final {
    auto next = Next();
    next.resume();
  }

  void Drop() noexcept final {
    this->Store(StopTag{});
    this->template SetResult<false>();
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  yaclib_std::coroutine_handle<> Next() noexcept final {
    auto handle = Handle();
    YACLIB_ASSERT(!handle.done());
    return handle;
  }
#endif
};

template <bool Lazy>
template <typename V, typename E>
void PromiseTypeDeleter<Lazy>::Delete(ResultCore<V, E>& core) noexcept {
  auto& promise = static_cast<PromiseType<V, E, Lazy>&>(core);
  auto handle = promise.Handle();
  handle.destroy();
}

}  // namespace yaclib::detail

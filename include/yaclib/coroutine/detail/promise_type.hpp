#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/coroutine_deleter.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib::detail {

template <typename V, typename E>
struct PromiseType;

template <typename V, typename E>
class Destroy {
 public:
  bool await_ready() noexcept {
    return false;
  }
  void await_resume() noexcept {
  }

  void await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) noexcept {
    handle.promise().DecRef();
  }
};

template <typename V, typename E>
struct PromiseType : AtomicCounter<ResultCore<V, E>, CoroutineDeleter> {
  using Base = AtomicCounter<ResultCore<V, E>, CoroutineDeleter>;
  PromiseType() : Base{2} {  // get_return_object is gonna be invoked right after ctor
  }

  Future<V, E> get_return_object() {
    return {ResultCorePtr<V, E>{NoRefTag{}, this}};
  }

  yaclib_std::suspend_never initial_suspend() noexcept {
    return {};
  }

  Destroy<V, E> final_suspend() noexcept {
    return {};
  }

  void return_value(const V& value) noexcept(std::is_nothrow_copy_constructible_v<V>) {
    Base::Set(value);
  }

  void return_value(V&& value) noexcept(std::is_nothrow_move_constructible_v<V>) {
    Base::Set(std::move(value));
  }

  void unhandled_exception() noexcept {
    Base::Set(std::current_exception());
  }
};

template <typename E>
struct PromiseType<void, E> : AtomicCounter<ResultCore<void, E>, CoroutineDeleter> {
  using Base = AtomicCounter<ResultCore<void, E>, CoroutineDeleter>;
  PromiseType() : Base{2} {  // get_return_object is gonna be invoked right after ctor
  }

  Future<void, E> get_return_object() noexcept {
    return {ResultCorePtr<void, E>{NoRefTag{}, this}};
  }

  yaclib_std::suspend_never initial_suspend() noexcept {
    return {};
  }

  Destroy<void, E> final_suspend() noexcept {
    return {};
  }

  void return_void() noexcept {
    Base::Set(Unit{});
  }

  void unhandled_exception() noexcept {
    Base::Set(std::current_exception());
  }
};

}  // namespace yaclib::detail

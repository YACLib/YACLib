#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/coro/coroutine.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <exception>

namespace yaclib::detail {

template <typename V, typename E>
class PromiseType;

template <typename V, typename E>
struct Destroy final {
  YACLIB_INLINE bool await_ready() const noexcept {
    return false;
  }

  YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) const noexcept {
    handle.promise().SetResult();
  }

  YACLIB_INLINE void await_resume() const noexcept {
  }
};

struct PromiseTypeDeleter {
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

  Destroy<V, E> final_suspend() noexcept {
    return {};
  }

  void unhandled_exception() noexcept {
    this->Store(std::current_exception());
  }

  /*
    TODO(MBkkt) Think about add zero-cost ability to return error
     now works only co_return MakeFuture(std::make_exception_ptr(...))
     and co_await On(stopped_executor) for StopTag{}
    Maybe:
     co_await yaclib::Stop();
     co_await yaclib::Stop(StopError{});
     co_await yaclib::Stop(std::make_exception_ptr(...));
  */

 private:
  void Call() noexcept final {
    auto handle = yaclib_std::coroutine_handle<PromiseType<V, E>>::from_promise(static_cast<PromiseType<V, E>&>(*this));
    YACLIB_DEBUG(!handle, "handle from promise is null");
    YACLIB_DEBUG(handle.done(), "handle for resume is done");
    handle.resume();
  }
};

template <typename V, typename E>
class PromiseType : public BasePromiseType<V, E> {
 public:
  void return_value(const V& value) noexcept(std::is_nothrow_copy_constructible_v<V>) {
    this->Store(value);
  }

  void return_value(V&& value) noexcept(std::is_nothrow_move_constructible_v<V>) {
    this->Store(std::move(value));
  }
};

template <typename E>
class PromiseType<void, E> : public BasePromiseType<void, E> {
 public:
  void return_void() noexcept {
    this->Store(Unit{});
  }
};

}  // namespace yaclib::detail

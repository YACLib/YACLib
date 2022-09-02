#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/helper.hpp>

namespace yaclib::detail {

enum class CoreType : char {
  Run = 0,
  Then = 1,
  Detach = 2,
};

template <CoreType Type, typename V, typename E>
using ResultCoreT = std::conditional_t<Type == CoreType::Detach, ResultCore<void, void>, ResultCore<V, E>>;

template <typename Ret, typename Arg, typename E, typename Wrapper, CoreType Type>
class Core : public ResultCoreT<Type, Ret, E> {
 public:
  using Base = ResultCoreT<Type, Ret, E>;

  template <typename Func>
  explicit Core(Func&& f) noexcept(std::is_nothrow_constructible_v<Wrapper, Func&&>) : _wrapper{std::forward<Func>(f)} {
    if constexpr (Wrapper::kIsAsync) {
      this->_unwrapping = 0;
    }
  }

 private:
  void Call() noexcept final {
    if constexpr (Wrapper::kIsAsync) {
      YACLIB_ASSERT(this->_unwrapping == 0);
      ++this->_unwrapping;
    }
    if constexpr (Type == CoreType::Run) {
      _wrapper.Call(*this, Unit{});
    } else {
      auto& core = static_cast<ResultCore<Arg, E>&>(*this->_caller);
      _wrapper.Call(*this, std::move(core.Get()));
    }
  }

 public:
  void Here(InlineCore& caller) noexcept final {
    if constexpr (Wrapper::kIsAsync) {
      if (this->_unwrapping++ != 0) {
        YACLIB_ASSERT(this->_unwrapping == 2);
        auto& core = static_cast<ResultCore<Ret, E>&>(caller);
        return _wrapper.Done(*this, std::move(core.Get()));
      }
    }
    auto& core = static_cast<ResultCore<Arg, E>&>(caller);
    _wrapper.Call(*this, std::move(core.Get()));
  }

 private:
  YACLIB_NO_UNIQUE_ADDRESS Wrapper _wrapper;
};

template <typename V, typename E, typename Func>
constexpr char Tag() noexcept {
  if constexpr (is_invocable_v<Func, Result<V, E>>) {
    return 1;
  } else if constexpr (is_invocable_v<Func, V>) {
    return 2;
  } else if constexpr (is_invocable_v<Func, E>) {
    return 4;
  } else if constexpr (is_invocable_v<Func, std::exception_ptr>) {
    return 8;
  } else {
    return 0;
  }
}

template <typename V, typename E, typename Func, char Tag = Tag<V, E, Func>()>
struct Return;

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 1> final {
  using Type = invoke_t<Func, Result<V, E>>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 2> final {
  using Type = invoke_t<Func, V>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 4> final {
  using Type = invoke_t<Func, E>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 8> final {
  using Type = invoke_t<Func, std::exception_ptr>;
};

template <bool Detach, bool Async, typename Ret, typename Arg, typename E, typename Func>
class FuncWrapper final {
  static_assert(!(Detach && Async), "Detach cannot be Async, should be void");

  using Store = std::decay_t<Func>;
  using Invoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Func>>, Store, Func>;
  using Core = std::conditional_t<Detach, ResultCore<void, void>, ResultCore<Ret, E>>;
  static constexpr bool kEarlyDestroy = true;

 public:
  static constexpr bool kIsAsync = Async;

  explicit FuncWrapper(Store&& f) noexcept(std::is_nothrow_move_constructible_v<Store>) {
    new (&_func.store) Store{std::move(f)};
  }

  explicit FuncWrapper(const Store& f) noexcept(std::is_nothrow_copy_constructible_v<Store>) {
    new (&_func.store) Store{f};
  }

  template <typename T>
  void Call(Core& self, T&& r) noexcept {
    try {
      constexpr bool kIsRun = std::is_same_v<T, Unit>;
      static_assert(kIsRun || std::is_same_v<T, Result<Arg, E>>);
      if constexpr (kIsRun || is_invocable_v<Invoke, Result<Arg, E>>) {
        CallResolveAsync(self, std::forward<T>(r));
      } else {
        CallResolveState(self, std::forward<T>(r));
      }
    } catch (...) {
      Done(self, std::current_exception());
    }
  }

  template <typename T>
  void Done(Core& self, T&& value) noexcept {
    self.Store(std::forward<T>(value));
    if constexpr (kEarlyDestroy) {
      _func.store.~Store();
    }
    self.template SetResult<false>();
  }

  ~FuncWrapper() noexcept {
    if constexpr (!kEarlyDestroy) {
      _func.store.~Store();
    }
  }

 private:
  void CallResolveState(Core& self, Result<Arg, E>&& r) {
    const auto state = r.State();
    if constexpr (is_invocable_v<Invoke, Arg>) {
      if (state == ResultState::Value) {
        CallResolveAsync(self, std::move(r).Value());
      } else if (state == ResultState::Exception) {
        Done(self, std::move(r).Exception());
      } else {
        YACLIB_DEBUG(state != ResultState::Error, "state should be Error, but here it's Empty");
        Done(self, std::move(r).Error());
      }
    } else {
      /**
       * We can't use this strategy for other Func::Invoke,
       * because in that case user will not have compile error for that case:
       * MakeFuture<>() // state == ResultState::Value
       *   .ThenInline([](E/std::exception_ptr) -> yaclib::Result/Future<double> {
       *     throw std::runtime_error{""};
       *   }).ThenInline([](yaclib::Result<double>) { // need double value, we only have void
       *     return 1;
       *   });
       * TLDR: Before and after the "recovery" callback must have the same value type
       */
      constexpr bool kIsException = is_invocable_v<Invoke, std::exception_ptr>;
      constexpr bool kIsError = is_invocable_v<Invoke, E>;
      static_assert(kIsException ^ kIsError, "Recovery callback should be invokable with std::exception_ptr or E");
      constexpr auto kState = kIsException ? ResultState::Exception : ResultState::Error;
      if (state == kState) {
        using T = std::conditional_t<kIsException, std::exception_ptr, E>;
        CallResolveAsync(self, std::move(r).template Extract<T>());
      } else {
        Done(self, std::move(r));
      }
    }
  }

  template <typename T>
  void CallResolveAsync(Core& self, T&& value) {
    if constexpr (Async) {
      auto future = CallResolveVoid(std::forward<T>(value));
      future.GetCore().Release()->SetInline(self);
    } else {
      Done(self, CallResolveVoid(std::forward<T>(value)));
    }
  }

  template <typename T>
  auto CallResolveVoid(T&& value) {
    constexpr bool kArgVoid = std::is_same_v<T, Unit>;
    constexpr bool kRetVoid = std::is_void_v<invoke_t<Invoke, std::conditional_t<kArgVoid, void, T>>>;
    if constexpr (kRetVoid) {
      if constexpr (kArgVoid) {
        std::forward<Invoke>(_func.store)();
      } else {
        std::forward<Invoke>(_func.store)(std::forward<T>(value));
      }
      return Unit{};
    } else if constexpr (kArgVoid) {
      return std::forward<Invoke>(_func.store)();
    } else {
      return std::forward<Invoke>(_func.store)(std::forward<T>(value));
    }
  }

  union State {
    YACLIB_NO_UNIQUE_ADDRESS Unit stub;
    YACLIB_NO_UNIQUE_ADDRESS Store store;

    State() {
    }

    ~State() {
    }
  };
  YACLIB_NO_UNIQUE_ADDRESS State _func;
};

template <CoreType CoreT, typename Arg, typename E, typename Func>
auto* MakeCore(Func&& f) {
  constexpr bool kIsDetach = CoreT == CoreType::Detach;
  static_assert(CoreT != CoreType::Run || std::is_void_v<Arg>,
                "It makes no sense to receive some value in first pipeline step");
  using AsyncRet = result_value_t<typename detail::Return<Arg, E, Func>::Type>;
  static_assert(!kIsDetach || std::is_void_v<AsyncRet>,
                "It makes no sense to return some value in Detach, since no one will be able to use it");
  using Ret = result_value_t<future_base_value_t<AsyncRet>>;
  constexpr bool kIsAsync = is_future_base_v<AsyncRet>;
  using Wrapper = detail::FuncWrapper<kIsDetach, kIsAsync, Ret, Arg, E, decltype(std::forward<Func>(f))>;
  // TODO(MBkkt) Think about inline/detach optimization
  using Core = detail::Core<Ret, Arg, E, Wrapper, CoreT>;
  return MakeUnique<Core>(std::forward<Func>(f)).Release();
}

enum class CallbackType : char {
  Inline = 0,
  InlineOn = 1,
  On = 2,
  Lazy = 3,
  LazyInline = 4,
};

[[nodiscard]] inline bool Detach(BaseCore& caller, InlineCore& callback = MakeEmpty()) noexcept {
  if (caller.SetCallback(callback, BaseCore::kWaitDrop)) {
    return true;
  }
  caller.DecRef();
  return false;
}

template <CoreType CoreT, CallbackType CallbackT, typename Arg, typename E, typename Func>
auto SetCallback(ResultCorePtr<Arg, E>& core, Func&& f) {
  constexpr bool kIsDetach = CoreT == CoreType::Detach;
  constexpr bool kIsTask = CallbackT == CallbackType::Lazy || CallbackT == CallbackType::LazyInline;
  auto* callback = MakeCore<CoreT, Arg, E>(std::forward<Func>(f));
  auto* caller = core.Release();
  if constexpr (!kIsDetach) {
    callback->_executor = move_if<(!kIsTask && CallbackT != CallbackType::On)>(caller->_executor);
  }
  if constexpr (CallbackT == CallbackType::Lazy) {
    callback->_caller = caller;
    caller->StoreCallback(*callback, BaseCore::kCall);
  } else if constexpr (CallbackT == CallbackType::LazyInline) {
    caller->StoreCallback(*callback, BaseCore::kInline);
  } else if constexpr (CallbackT == CallbackType::On) {
    caller->SetCall(*callback);
  } else {
    caller->SetInline(*callback);
  }
  using ResultCoreT = typename std::remove_reference_t<decltype(*callback)>::Base;
  if constexpr (kIsTask) {
    callback->next = caller;
    return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
  } else if constexpr (kIsDetach) {
    (void)Detach(*callback);
  } else {
    static_assert(CoreT == CoreType::Then, "SetCallback don't works for Run");
    if constexpr (CallbackT == CallbackType::Inline) {
      return Future{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
    } else {
      return FutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
    }
  }
}

}  // namespace yaclib::detail

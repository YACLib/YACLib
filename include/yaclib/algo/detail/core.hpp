#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/helper.hpp>

namespace yaclib::detail {

class NoResultCore : public BaseCore {
 public:
  NoResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename T>
  void Store(T&&) noexcept {
  }

  Callback _self;
};

enum class CoreType : char {
  Run = 0,
  Then = 1,
  Detach = 2,
};

template <CoreType Type, typename V, typename E>
using ResultCoreT = std::conditional_t<Type == CoreType::Detach, NoResultCore, ResultCore<V, E>>;

template <typename Ret, typename Arg, typename E, typename Func, CoreType Type, bool Async>
class Core : public ResultCoreT<Type, Ret, E> {
  using Storage = std::decay_t<Func>;
  using Invoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Func>>, Storage, Func>;

  static_assert(!(Type == CoreType::Detach && Async), "Detach cannot be Async, should be void");

 public:
  using Base = ResultCoreT<Type, Ret, E>;

  explicit Core(Storage&& f) noexcept(std::is_nothrow_move_constructible_v<Storage>) {
    this->_self = Callback{&MakeEmpty(), 0};
    new (&_func.storage) Storage{std::move(f)};
  }

  explicit Core(const Storage& f) noexcept(std::is_nothrow_copy_constructible_v<Storage>) {
    this->_self = Callback{&MakeEmpty(), 0};
    new (&_func.storage) Storage{std::move(f)};
  }

 private:
  void Tick() noexcept {
    if constexpr (Async) {
      YACLIB_ASSERT(this->_self.unwrapping == 0);
      this->_self.unwrapping = 1;
    }
  }

  void Call() noexcept final {
    Tick();
    if constexpr (Type == CoreType::Run) {
      CallImpl(Unit{});
    } else {
      auto& core = static_cast<ResultCore<Arg, E>&>(*this->_self.caller);
      CallImpl(std::move(core.Get()));
    }
  }

  void Drop() noexcept final {
    Tick();
    CallImpl(Result<Arg, E>{StopTag{}});
  }

  void Here(BaseCore& caller) noexcept final {
    if constexpr (Async) {
      if (this->_self.unwrapping++ != 0) {
        YACLIB_ASSERT(this->_self.unwrapping == 2);
        auto& core = static_cast<ResultCore<Ret, E>&>(caller);
        Done(std::move(core.Get()));
        caller.DecRef();
        return;
      }
    }
    if constexpr (Type != CoreType::Run) {
      if (!this->_executor) {
        this->_executor = std::move(caller._executor);
      }
    }
    YACLIB_ASSERT(this->_executor);
    auto& core = static_cast<ResultCore<Arg, E>&>(caller);
    CallImpl(std::move(core.Get()));
    caller.DecRef();
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    Here(caller);
    return yaclib_std::noop_coroutine();
  }
#endif

  template <typename T>
  void CallImpl(T&& r) noexcept try {
    if constexpr (std::is_same_v<T, Unit> || is_invocable_v<Invoke, Result<Arg, E>>) {
      CallResolveAsync(std::forward<T>(r));
    } else {
      CallResolveState(std::forward<T>(r));
    }
  } catch (...) {
    Done(std::current_exception());
  }

  template <typename T>
  void Done(T&& value) noexcept {
    auto* caller = this->_self.caller;
    this->Store(std::forward<T>(value));
    // TODO(MBkkt) What order is better here?
    // Now it's construct return object, then destroy and dealloc argument, and then destroy current callback functor
    if constexpr (Type != CoreType::Run) {
      caller->DecRef();
    }
    _func.storage.~Storage();
    this->template SetResult<false>();
  }

  void CallResolveState(Result<Arg, E>&& r) {
    const auto state = r.State();
    if constexpr (is_invocable_v<Invoke, Arg> || (std::is_void_v<Arg> && is_invocable_v<Invoke, Unit>)) {
      if (state == ResultState::Value) {
        CallResolveAsync(std::move(r).Value());
      } else if (state == ResultState::Exception) {
        Done(std::move(r).Exception());
      } else {
        YACLIB_ASSERT(state == ResultState::Error);
        Done(std::move(r).Error());
      }
    } else {
      /**
       * TLDR: Before and after the "recovery" callback must have the same value type
       *
       * Why can't we use the above strategy for other Invoke?
       * Because user will not have compile error for this case:
       * MakeFuture()
       *   // Can't call next recovery callback, because our result is Ok, so we will skip next callback
       *   .ThenInline([](E/std::exception_ptr) -> yaclib::Result/Future<double> {
       *     throw std::runtime_error{""};
       *   })
       *   // Previous callback was skipped, so previous Result type is void (received from MakeFuture)
       *   // But here we need Result<double>, so it leeds error
       *   .ThenInline([](yaclib::Result<double>) {
       *     return 1;
       *   });
       */
      constexpr bool kIsException = is_invocable_v<Invoke, std::exception_ptr>;
      constexpr bool kIsError = is_invocable_v<Invoke, E>;
      static_assert(kIsException ^ kIsError, "Recovery callback should be invokable with std::exception_ptr or E");
      constexpr auto kState = kIsException ? ResultState::Exception : ResultState::Error;
      if (state == kState) {
        using T = std::conditional_t<kIsException, std::exception_ptr, E>;
        CallResolveAsync(std::move(r).template Extract<T>());
      } else {
        Done(std::move(r));
      }
    }
  }

  template <typename T>
  void CallResolveAsync(T&& value) {
    if constexpr (Async) {
      auto future = CallResolveVoid(std::forward<T>(value));
      future.GetCore().Release()->SetInline(*this);
    } else {
      Done(CallResolveVoid(std::forward<T>(value)));
    }
  }

  template <typename T>
  auto CallResolveVoid(T&& value) {
    constexpr bool kArgVoid = is_invocable_v<Invoke, void>;
    constexpr bool kRetVoid = std::is_void_v<invoke_t<Invoke, std::conditional_t<kArgVoid, void, T>>>;
    if constexpr (kRetVoid) {
      if constexpr (kArgVoid) {
        std::forward<Invoke>(_func.storage)();
      } else {
        std::forward<Invoke>(_func.storage)(std::forward<T>(value));
      }
      return Unit{};
    } else if constexpr (kArgVoid) {
      return std::forward<Invoke>(_func.storage)();
    } else {
      return std::forward<Invoke>(_func.storage)(std::forward<T>(value));
    }
  }

  union State {
    YACLIB_NO_UNIQUE_ADDRESS Unit stub;
    YACLIB_NO_UNIQUE_ADDRESS Storage storage;

    State() noexcept {
    }

    ~State() noexcept {
    }
  };
  YACLIB_NO_UNIQUE_ADDRESS State _func;
};

template <typename V, typename E, typename Func>
constexpr char Tag() noexcept {
  if constexpr (is_invocable_v<Func, Result<V, E>>) {
    return 1;
  } else if constexpr (is_invocable_v<Func, V>) {
    return 2;
  } else if constexpr (is_invocable_v<Func, E>) {
    return 3;
  } else if constexpr (is_invocable_v<Func, std::exception_ptr>) {
    return 4;
  } else if constexpr (is_invocable_v<Func, Unit>) {
    return 5;
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
struct Return<V, E, Func, 3> final {
  using Type = invoke_t<Func, E>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 4> final {
  using Type = invoke_t<Func, std::exception_ptr>;
};

template <typename V, typename E, typename Func>
struct Return<V, E, Func, 5> final {
  using Type = invoke_t<Func, Unit>;
};

template <CoreType CoreT, typename Arg0, typename E, typename Func>
auto* MakeCore(Func&& f) {
  using Arg = std::conditional_t<std::is_same_v<Arg0, Unit>, void, Arg0>;
  static_assert(CoreT != CoreType::Run || std::is_void_v<Arg>,
                "It makes no sense to receive some value in first pipeline step");
  using AsyncRet = result_value_t<typename detail::Return<Arg, E, Func&&>::Type>;
  static_assert(CoreT != CoreType::Detach || std::is_void_v<AsyncRet>,
                "It makes no sense to return some value in Detach, since no one will be able to use it");
  using Ret = result_value_t<future_base_value_t<AsyncRet>>;
  constexpr bool kIsAsync = is_future_base_v<AsyncRet>;
  // TODO(MBkkt) Think about inline/detach optimization
  using Core = detail::Core<Ret, Arg, E, Func&&, CoreT, kIsAsync>;
  return MakeUnique<Core>(std::forward<Func>(f)).Release();
}

enum class CallbackType : char {
  Inline = 0,
  InlineOn = 1,
  On = 2,
  Lazy = 3,
  LazyInline = 4,
};

template <CoreType CoreT, CallbackType CallbackT, typename Arg, typename E, typename Func>
auto SetCallback(ResultCorePtr<Arg, E>& core, IExecutor* executor, Func&& f) {
  constexpr bool kIsDetach = CoreT == CoreType::Detach;
  constexpr bool kIsTask = CallbackT == CallbackType::Lazy || CallbackT == CallbackType::LazyInline;
  auto* callback = MakeCore<CoreT, Arg, E>(std::forward<Func>(f));
  // TODO(MBkkt) callback, executor, caller should be in ctor
  if constexpr (kIsDetach) {
    callback->StoreCallback(detail::MakeDrop(), detail::BaseCore::kInline);
  }
  callback->_executor = executor;
  auto* caller = core.Release();
  if constexpr (CallbackT == CallbackType::Lazy) {
    callback->_self.caller = caller;
    caller->StoreCallback(*callback, BaseCore::kCall);
  } else if constexpr (CallbackT == CallbackType::LazyInline) {
    caller->StoreCallback(*callback, BaseCore::kInline);
  } else if constexpr (CallbackT == CallbackType::On) {
    callback->_self.caller = caller;
    caller->SetCall(*callback);
  } else {
    caller->SetInline(*callback);
  }

  using ResultCoreT = typename std::remove_reference_t<decltype(*callback)>::Base;
  if constexpr (kIsTask) {
    callback->next = caller;
    return Task{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
  } else if constexpr (!kIsDetach) {
    static_assert(CoreT == CoreType::Then, "SetCallback don't works for Run");
    if constexpr (CallbackT == CallbackType::Inline) {
      return Future{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
    } else {
      return FutureOn{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
    }
  }
}

}  // namespace yaclib::detail

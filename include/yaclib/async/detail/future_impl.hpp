#pragma once

#include <yaclib/algo/wait.hpp>
#include <yaclib/async/detail/core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib {
namespace detail {

template <typename V, typename E, typename Functor>
constexpr int Tag() {
  int tag = 0;
  if constexpr (is_invocable_v<Functor, Result<V, E>>) {
    tag += 1;
  } else if constexpr (is_invocable_v<Functor, V>) {
    tag += 2;
  } else if constexpr (is_invocable_v<Functor, E>) {
    tag += 4;
  } else if constexpr (is_invocable_v<Functor, std::exception_ptr>) {
    tag += 8;
  }
  return tag;
}

template <typename V, typename E, typename Functor, int Tag = Tag<V, E, Functor>()>
struct Return;

template <typename V, typename E, typename Functor>
struct Return<V, E, Functor, 1> {
  using Type = invoke_t<Functor, Result<V, E>>;
};

template <typename V, typename E, typename Functor>
struct Return<V, E, Functor, 2> {
  using Type = invoke_t<Functor, V>;
};

template <typename V, typename E, typename Functor>
struct Return<V, E, Functor, 4> {
  using Type = invoke_t<Functor, E>;
};

template <typename V, typename E, typename Functor>
struct Return<V, E, Functor, 8> {
  using Type = invoke_t<Functor, std::exception_ptr>;
};

template <bool Subscribe, bool Async, typename Ret, typename Arg, typename E, typename Functor>
class FunctorWrapper {
  //  using FunctorStore = std::remove_cv_t<std::remove_reference_t<Functor>>;
  //  using FunctorInvoke = Functor;
  using Store = std::decay_t<Functor>;
  using Invoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Functor>>, Store, Functor>;
  using Core = std::conditional_t<Subscribe, ResultCore<void, void>, ResultCore<Ret, E>>;

 public:
  static constexpr bool kIsAsync = Async;

  explicit FunctorWrapper(Store&& f) noexcept(std::is_nothrow_move_constructible_v<Store>) : _f{std::move(f)} {
  }

  explicit FunctorWrapper(const Store& f) noexcept(std::is_nothrow_copy_constructible_v<Store>) : _f{f} {
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
      self.Set(std::current_exception());
    }
  }

 private:
  void CallResolveState(Core& self, Result<Arg, E>&& r) {
    const auto state = r.State();
    if constexpr (is_invocable_v<Invoke, Arg>) {
      if (state == ResultState::Value) {
        CallResolveAsync(self, std::move(r).Value());
      } else if (state == ResultState::Error) {
        self.Set(std::move(r).Error());
      } else {
        assert(state == ResultState::Exception);
        self.Set(std::move(r).Exception());
      }
    } else {
      /**
       * We can't use this strategy for other Functor::Invoke,
       * because in that case user will not have compile error for that case:
       * MakeFuture<void>() // state == ResultState::Value
       *   .ThenInline([](E/std::exception_ptr) -> yaclib::Result/Future<double> {
       *     throw std::runtime_error{""};
       *   }).ThenInline([](yaclib::Result<double>) { // need double value, we only have void
       *     return 1;
       *   });
       * TLDR: Before and after the "recovery" callback must have the same value type
       */
      constexpr bool kIsError = is_invocable_v<Invoke, E>;
      constexpr bool kIsException = is_invocable_v<Invoke, std::exception_ptr>;
      static_assert(kIsError ^ kIsException, "Recovery callback should be invokable with E or std::exception_ptr");
      constexpr auto kState = kIsError ? ResultState::Error : ResultState::Exception;
      if (state == kState) {
        using T = std::conditional_t<kIsError, E, std::exception_ptr>;
        CallResolveAsync(self, std::move(r).template Extract<T>());
      } else {
        self.Set(std::move(r));
      }
    }
  }

  template <typename T>
  void CallResolveAsync(Core& self, T&& value) {
    if constexpr (Async) {
      auto future = CallResolveVoid(std::forward<T>(value));
      auto core = std::exchange(future.GetCore(), nullptr);
      self.IncRef();
      core->SetCallbackInline(self, true);
    } else {
      self.Set(CallResolveVoid(std::forward<T>(value)));
    }
  }

  template <typename T>
  auto CallResolveVoid(T&& value) {
    constexpr bool kArgVoid = std::is_same_v<T, Unit>;
    constexpr bool kRetVoid = std::is_void_v<invoke_t<Invoke, std::conditional_t<kArgVoid, void, T>>>;
    if constexpr (kRetVoid) {
      if constexpr (kArgVoid) {
        std::forward<Invoke>(_f)();
      } else {
        std::forward<Invoke>(_f)(std::forward<T>(value));
      }
      return Unit{};
    } else if constexpr (kArgVoid) {
      return std::forward<Invoke>(_f)();
    } else {
      return std::forward<Invoke>(_f)(std::forward<T>(value));
    }
  }

  Store _f;
};

template <CoreType Type, typename Arg, typename E, typename Functor>
auto* MakeCore(Functor&& f) {
  static_assert(Type != CoreType::Run || std::is_void_v<Arg>, "It makes no sense to receive some value in first step");
  using AsyncRet = result_value_t<typename detail::Return<Arg, E, Functor>::Type>;
  static_assert(Type != CoreType::Subscribe || std::is_void_v<AsyncRet>,
                "It makes no sense to return some value in Subscribe, since no one will be able to use it");
  using Ret = result_value_t<future_value_t<AsyncRet>>;
  constexpr bool kIsAsync = is_future_v<AsyncRet>;
  using Wrapper =
    detail::FunctorWrapper<Type == CoreType::Subscribe, kIsAsync, Ret, Arg, E, decltype(std::forward<Functor>(f))>;
  // TODO(MBkkt) Think about inline/subscribe optimization
  using Core = detail::Core<Ret, Arg, E, Wrapper, Type>;
  constexpr std::size_t kRef = 1 + static_cast<std::size_t>(Type != CoreType::Subscribe);
  return new detail::AtomicCounter<Core>{kRef, std::forward<Functor>(f)};
}

template <CoreType Type, bool Inline, typename Arg, typename E, typename Functor>
auto SetCallback(ResultCorePtr<Arg, E> caller, Functor&& f) {
  static_assert(Type != CoreType::Run);
  auto* callback = MakeCore<Type, Arg, E>(std::forward<Functor>(f));
  callback->SetExecutor(caller->GetExecutor());
  if constexpr (Inline) {
    caller->SetCallbackInline(*callback);
  } else {
    caller->SetCallback(*callback);
  }
  if constexpr (Type == CoreType::Then) {  // TODO(MBkkt) exception safety?
    using ResultCoreT = typename std::remove_reference_t<decltype(*callback)>::Base;
    return Future{IntrusivePtr<ResultCoreT>{NoRefTag{}, callback}};
  }
}

}  // namespace detail

template <typename V, typename E>
Future<V, E>::~Future() {
  if (_core) {
    _core->Stop();
  }
}

template <typename V, typename E>
bool Future<V, E>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename V, typename E>
bool Future<V, E>::Ready() const& noexcept {
  return !_core->Empty();
}

template <typename V, typename E>
const Result<V, E>* Future<V, E>::Get() const& noexcept {
  if (Ready()) {  // TODO(MBkkt) Maybe we want likely
    return &_core->Get();
  }
  return nullptr;
}

template <typename V, typename E>
Result<V, E> Future<V, E>::Get() && noexcept {
  if (!Ready()) {
    Wait(*this);
  }
  auto core = std::exchange(_core, nullptr);
  return std::move(core->Get());
}

template <typename V, typename E>
const Result<V, E>& Future<V, E>::GetUnsafe() const& noexcept {
  assert(Ready());
  return _core->Get();
}

template <typename V, typename E>
Result<V, E> Future<V, E>::GetUnsafe() && noexcept {
  assert(Ready());
  auto core = std::exchange(_core, nullptr);
  return std::move(core->Get());
}

template <typename V, typename E>
void Future<V, E>::Stop() && {
  _core->Stop();
  _core = nullptr;
}

template <typename V, typename E>
void Future<V, E>::Detach() && {
  _core = nullptr;
}

template <typename V, typename E>
Future<V, E>&& Future<V, E>::Via(IExecutorPtr e) && {
  _core->SetExecutor(std::move(e));
  return std::move(*this);
}

template <typename V, typename E>
template <typename Functor>
auto Future<V, E>::Then(Functor&& f) && {
  return detail::SetCallback<detail::CoreType::Then, false>(std::move(_core), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
auto Future<V, E>::ThenInline(Functor&& f) && {
  return detail::SetCallback<detail::CoreType::Then, true>(std::move(_core), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
auto Future<V, E>::Then(IExecutorPtr e, Functor&& f) && {
  YACLIB_ERROR(e == nullptr, "nullptr executor supplied");
  YACLIB_INFO(e->Tag() == IExecutor::Type::Inline,
              "better way is use ThenInline(...) instead of Then(MakeInline(), ...)");
  _core->SetExecutor(std::move(e));
  return detail::SetCallback<detail::CoreType::Then, false>(std::move(_core), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
void Future<V, E>::Subscribe(Functor&& f) && {
  detail::SetCallback<detail::CoreType::Subscribe, false>(std::move(_core), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
void Future<V, E>::SubscribeInline(Functor&& f) && {
  detail::SetCallback<detail::CoreType::Subscribe, true>(std::move(_core), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
void Future<V, E>::Subscribe(IExecutorPtr e, Functor&& f) && {
  YACLIB_ERROR(e == nullptr, "nullptr executor supplied");
  YACLIB_INFO(e->Tag() == IExecutor::Type::Inline,
              "better way is use ThenInline(...) instead of Then(MakeInline(), ...)");
  _core->SetExecutor(std::move(e));
  detail::SetCallback<detail::CoreType::Subscribe, false>(std::move(_core), std::forward<Functor>(f));
}

template <typename V, typename E>
Future<V, E>::Future(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& Future<V, E>::GetCore() noexcept {
  return _core;
}

}  // namespace yaclib

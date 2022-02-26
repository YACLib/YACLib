#pragma once

#include <yaclib/algo/wait.hpp>
#include <yaclib/async/detail/core.hpp>

#include <cassert>

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

template <typename Ret, typename Arg, typename E, typename Functor>
class SyncInvoke {
  //  using FunctorStore = std::remove_cv_t<std::remove_reference_t<Functor>>;
  //  using FunctorInvoke = Functor;
  using FunctorStore = std::decay_t<Functor>;
  using FunctorInvoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Functor>>, FunctorStore, Functor>;

 public:
  static constexpr bool kIsAsync = false;

  explicit SyncInvoke(FunctorStore&& f) noexcept(std::is_nothrow_move_constructible_v<FunctorStore>)
      : _f{std::move(f)} {
  }

  explicit SyncInvoke(const FunctorStore& f) noexcept(std::is_nothrow_copy_constructible_v<FunctorStore>) : _f{f} {
  }

  void Wrapper(ResultCore<Ret, E>& self, Result<Arg, E>&& r) noexcept {
    try {
      if constexpr (is_invocable_v<FunctorInvoke, Result<Arg, E>>) {
        return WrapperResult(self, std::move(r));
      } else {
        return WrapperOther(self, std::move(r));
      }
    } catch (...) {
      return self.Set(std::current_exception());
    }
  }

 private:
  template <typename T>
  void WrapperResult(ResultCore<Ret, E>& self, T&& value) {
    if constexpr (std::is_void_v<invoke_t<FunctorInvoke, T>>) {
      std::forward<FunctorInvoke>(_f)(std::forward<T>(value));
      return self.Set(Unit{});
    } else {
      return self.Set(std::forward<FunctorInvoke>(_f)(std::forward<T>(value)));
    }
  }

  void WrapperOther(ResultCore<Ret, E>& self, Result<Arg, E>&& r) {
    const auto state = r.State();
    if constexpr (is_invocable_v<FunctorInvoke, Arg>) {
      if (state == ResultState::Value) {
        if constexpr (std::is_void_v<Arg>) {
          if constexpr (std::is_void_v<invoke_t<FunctorInvoke, Arg>>) {
            std::forward<FunctorInvoke>(_f)();
            return self.Set(Unit{});
          } else {
            return self.Set(std::forward<FunctorInvoke>(_f)());
          }
        } else {
          return WrapperResult(self, std::move(r).Value());
        }
      }
      if (state == ResultState::Error) {
        return self.Set(std::move(r).Error());
      }
      assert(state == ResultState::Exception);
      return self.Set(std::move(r).Exception());
    } else {
      constexpr bool kIsError = is_invocable_v<FunctorInvoke, E>;
      constexpr ResultState kState = kIsError ? ResultState::Error : ResultState::Exception;
      using T = std::conditional_t<kIsError, E, std::exception_ptr>;
      if (state == kState) {
        return WrapperResult(self, std::move(r).template Extract<T>());
      }
      return self.Set(std::move(r));
    }
  }

  FunctorStore _f;
};

template <typename Ret, typename Arg, typename E, typename Functor>
class AsyncInvoke {
  using FunctorStore = std::decay_t<Functor>;
  using FunctorInvoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Functor>>, FunctorStore, Functor>;

 public:
  static constexpr bool kIsAsync = true;

  explicit AsyncInvoke(FunctorStore&& f) noexcept(std::is_nothrow_move_constructible_v<FunctorStore>)
      : _f{std::move(f)} {
  }

  explicit AsyncInvoke(const FunctorStore& f) noexcept(std::is_nothrow_copy_constructible_v<FunctorStore>) : _f{f} {
  }

  void Wrapper(ResultCore<Ret, E>& self, Result<Arg, E>&& r) noexcept {
    try {
      if constexpr (is_invocable_v<FunctorInvoke, Result<Arg, E>>) {
        return WrapperSubscribe(self, std::move(r));
      } else {
        return WrapperOther(self, std::move(r));
      }
    } catch (...) {
      return self.Set(std::current_exception());
    }
  }

 private:
  template <typename T>
  void WrapperSubscribe(ResultCore<Ret, E>& self, T&& value) {
    auto future = std::forward<FunctorInvoke>(_f)(std::forward<T>(value));
    self.IncRef();
    std::exchange(future.GetCore(), nullptr)->SetCallbackInline(self, true);
  }

  void WrapperOther(ResultCore<Ret, E>& self, Result<Arg, E>&& r) {
    const auto state = r.State();
    if constexpr (is_invocable_v<FunctorInvoke, Arg>) {
      if (state == ResultState::Value) {
        return WrapperSubscribe(self, std::move(r).Value());
      }
      if (state == ResultState::Error) {
        return self.Set(std::move(r).Error());
      }
      assert(state == ResultState::Exception);
      return self.Set(std::move(r).Exception());
      /**
       * We can't use this strategy for other FunctorInvoke,
       * because in that case user will not have compile error for that case:
       * MakeFuture(32) // state == ResultState::Value
       *   .ThenInline([](E/std::exception_ptr) -> yaclib::Future<double> {
       *     throw std::runtime_error{""};
       *   })
       *   .ThenInline([](yaclib::Result<double>) { // need double Value
       *     return 1;
       *   });
       * TLDR: Before and after the "recovery" callback must have the same value type
       */
    } else {
      constexpr bool kIsError = is_invocable_v<FunctorInvoke, E>;
      constexpr ResultState kState = kIsError ? ResultState::Error : ResultState::Exception;
      using T = std::conditional_t<kIsError, E, std::exception_ptr>;
      if (state == kState) {
        return WrapperSubscribe(self, std::move(r).template Extract<T>());
      }
      return self.Set(std::move(r));
    }
  }

  FunctorStore _f;
};

template <bool Subscribe, bool Inline, typename Arg, typename E, typename Functor>
auto SetCallback(ResultCorePtr<Arg, E> caller, Functor&& f) {
  // TODO(kononovk/MBkkt): think about how to distinguish first and second overloads,
  //  when T is constructable from Result
  // TODO(myannyax) info if subscribe and Ret != void
  using AsyncRet = result_value_t<typename detail::Return<Arg, E, Functor>::Type>;
  static_assert(!Subscribe || std::is_void_v<AsyncRet>,
                "It makes no sense to return some value in Subscribe, since no one will be able to use it");
  using Ret = std::conditional_t<Subscribe, detail::SubscribeTag, result_value_t<future_value_t<AsyncRet>>>;
  constexpr bool kIsAsync = !Subscribe && is_future_v<AsyncRet>;
  using Invoke = std::conditional_t<kIsAsync,  //
                                    detail::AsyncInvoke<Ret, Arg, E, decltype(std::forward<Functor>(f))>,
                                    detail::SyncInvoke<Ret, Arg, E, decltype(std::forward<Functor>(f))>>;
  // TODO(MBkkt) Think about inline/subscribe optimization, something like
  // if constexpr (Subscribe && Inline && !kIsAsync) {
  //   if (!caller->Empty()) {
  //     Invoke invoke{std::forward<Functor>(f)};
  //     return invoke.Wrapper(nullptr, caller->Get());
  //   }
  // }
  using Core = detail::Core<Ret, Arg, E, Invoke>;
  auto callback = new detail::AtomicCounter<Core>{1 + static_cast<std::size_t>(!Subscribe), std::forward<Functor>(f)};
  callback->SetExecutor(caller->GetExecutor());
  if constexpr (Inline) {
    caller->SetCallbackInline(*callback);
  } else {
    caller->SetCallback(*callback);
  }
  if constexpr (!Subscribe) {  // TODO(MBkkt) exception safety?
    return Future<Ret, E>{IntrusivePtr{NoRefTag{}, callback}};
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
Future<V, E>& Future<V, E>::Via(IExecutorPtr e) & {
  _core->SetExecutor(std::move(e));
  return *this;
}

template <typename V, typename E>
Future<V, E>&& Future<V, E>::Via(IExecutorPtr e) && {
  return std::move(Via(std::move(e)));
}

template <typename V, typename E>
template <typename Functor>
auto Future<V, E>::Then(Functor&& f) && {
  return detail::SetCallback<false, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
auto Future<V, E>::ThenInline(Functor&& f) && {
  return detail::SetCallback<false, true>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
auto Future<V, E>::Then(IExecutorPtr e, Functor&& f) && {
  assert(e);
  // assert(e->Tag() != IExecutor::Type::Inline); TODO(maynnyax) info
  _core->SetExecutor(std::move(e));
  return detail::SetCallback<false, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
void Future<V, E>::Subscribe(Functor&& f) && {
  detail::SetCallback<true, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
void Future<V, E>::SubscribeInline(Functor&& f) && {
  detail::SetCallback<true, true>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename V, typename E>
template <typename Functor>
void Future<V, E>::Subscribe(IExecutorPtr e, Functor&& f) && {
  assert(e);
  // assert(e->Tag() != IExecutor::Type::Inline); TODO(maynnyax) info
  _core->SetExecutor(std::move(e));
  detail::SetCallback<true, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename V, typename E>
Future<V, E>::Future(detail::ResultCorePtr<V, E> core) noexcept : _core{std::move(core)} {
}

template <typename V, typename E>
detail::ResultCorePtr<V, E>& Future<V, E>::GetCore() noexcept {
  return _core;
}

}  // namespace yaclib

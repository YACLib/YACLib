#pragma once

#ifndef YACLIB_ASYNC_IMPL
#error "You can not include this header directly, use yaclib/async/future.hpp"
#endif

#include <yaclib/algo/wait.hpp>
#include <yaclib/async/detail/core.hpp>

#include <cassert>

namespace yaclib {
namespace detail {

template <typename T, typename Functor>
constexpr int Tag() {
  int tag{0};
  if constexpr (util::IsInvocableV<Functor, util::Result<T>>) {
    tag += 1;
  } else if constexpr (util::IsInvocableV<Functor, T>) {
    tag += 2;
  } else if constexpr (util::IsInvocableV<Functor, std::error_code>) {
    tag += 4;
  } else if constexpr (util::IsInvocableV<Functor, std::exception_ptr>) {
    tag += 8;
  }
  return tag;
}

template <typename T, typename Functor, int Tag = Tag<T, Functor>()>
struct Return;

template <typename T, typename Functor>
struct Return<T, Functor, 1> {
  using Type = util::InvokeT<Functor, util::Result<T>>;
};

template <typename T, typename Functor>
struct Return<T, Functor, 2> {
  using Type = util::InvokeT<Functor, T>;
};

template <typename T, typename Functor>
struct Return<T, Functor, 4> {
  using Type = util::InvokeT<Functor, std::error_code>;
};

template <typename T, typename Functor>
struct Return<T, Functor, 8> {
  using Type = util::InvokeT<Functor, std::exception_ptr>;
};

template <typename U, typename FunctorT, typename T>
class SyncInvoke {
  using FunctorStoreT = std::decay_t<FunctorT>;
  using FunctorInvokeT =
      std::conditional_t<std::is_function_v<std::remove_reference_t<FunctorT>>, FunctorStoreT, FunctorT>;

 public:
  explicit SyncInvoke(FunctorStoreT&& f) noexcept(std::is_nothrow_move_constructible_v<FunctorStoreT>)
      : _f{std::move(f)} {
  }

  explicit SyncInvoke(const FunctorStoreT& f) noexcept(std::is_nothrow_copy_constructible_v<FunctorStoreT>) : _f{f} {
  }

  util::Result<U> Wrapper(util::Result<T>&& r) noexcept {
    try {
      if constexpr (util::IsInvocableV<FunctorInvokeT, util::Result<T>>) {
        return WrapperResult(std::move(r));
      } else {
        return WrapperOther(std::move(r));
      }
    } catch (...) {
      return {std::current_exception()};
    }
  }

 private:
  util::Result<U> WrapperResult(util::Result<T>&& r) {
    if constexpr (std::is_void_v<U> && !util::IsResultV<util::InvokeT<FunctorInvokeT, util::Result<T>>>) {
      std::forward<FunctorInvokeT>(_f)(std::move(r));
      return util::Result<U>::Default();
    } else {
      return {std::forward<FunctorInvokeT>(_f)(std::move(r))};
    }
  }

  util::Result<U> WrapperOther(util::Result<T>&& r) {
    const auto state = r.State();
    if constexpr (util::IsInvocableV<FunctorInvokeT, T>) {
      if (state == util::ResultState::Value) {
        if constexpr (std::is_void_v<U> && !util::IsResultV<util::InvokeT<FunctorInvokeT, T>>) {
          if constexpr (std::is_void_v<T>) {
            std::forward<FunctorInvokeT>(_f)();
          } else {
            std::forward<FunctorInvokeT>(_f)(std::move(r).Value());
          }
          return util::Result<U>::Default();
        } else if constexpr (std::is_void_v<T>) {
          return {std::forward<FunctorInvokeT>(_f)()};
        } else {
          return {std::forward<FunctorInvokeT>(_f)(std::move(r).Value())};
        }
      }
      if (state == util::ResultState::Error) {
        return {std::move(r).Error()};
      }
      assert(state == util::ResultState::Exception);
      return {std::move(r).Exception()};
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::error_code>) {
      if (state == util::ResultState::Error) {
        return {std::forward<FunctorInvokeT>(_f)(std::move(r).Error())};
      }
      return std::move(r);
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::exception_ptr>) {
      if (state == util::ResultState::Exception) {
        return {std::forward<FunctorInvokeT>(_f)(std::move(r).Exception())};
      }
      return std::move(r);
    }
  }

  FunctorStoreT _f;
};

template <typename U, typename FunctorT, typename T>
class AsyncInvoke {
  using FunctorStoreT = std::decay_t<FunctorT>;
  using FunctorInvokeT =
      std::conditional_t<std::is_function_v<std::remove_reference_t<FunctorT>>, FunctorStoreT, FunctorT>;

 public:
  explicit AsyncInvoke(detail::PromiseCorePtr<U> promise,
                       FunctorStoreT&& f) noexcept(std::is_nothrow_move_constructible_v<FunctorStoreT>)
      : _promise{std::move(promise)}, _f{std::move(f)} {
  }

  explicit AsyncInvoke(detail::PromiseCorePtr<U> promise,
                       const FunctorStoreT& f) noexcept(std::is_nothrow_copy_constructible_v<FunctorStoreT>)
      : _promise{std::move(promise)}, _f{f} {
  }

  util::Result<void> Wrapper(util::Result<T>&& r) noexcept {
    try {
      if constexpr (util::IsInvocableV<FunctorInvokeT, util::Result<T>>) {
        WrapperSubscribe(std::move(r));
      } else {
        WrapperOther(std::move(r));
      }
    } catch (...) {
      std::move(_promise).Set(std::current_exception());
    }
    return util::Result<void>::Default();
  }

 private:
  template <typename Arg>
  void WrapperSubscribe(Arg&& a) {
    std::forward<FunctorInvokeT>(_f)(std::forward<Arg>(a))
        .SubscribeInline([promise = std::move(_promise)](util::Result<U>&& r) mutable {
          std::move(promise).Set(std::move(r));
        });
  }

  void WrapperOther(util::Result<T>&& r) {
    const auto state = r.State();
    if constexpr (util::IsInvocableV<FunctorInvokeT, T>) {
      if (state == util::ResultState::Value) {
        return WrapperSubscribe(std::move(r).Value());
      }
      if (state == util::ResultState::Error) {
        return std::move(_promise).Set(std::move(r).Error());
      }
      assert(state == util::ResultState::Exception);
      return std::move(_promise).Set(std::move(r).Exception());
      /** We can't use this strategy for other FunctorInvokeT,
       *  because in that case user will not have compiled error for that case:
       *  yaclib::MakeFuture(32) // state == util::ResultState::Value
       *    .ThenInline([](std::exception_ptr/std::error_code) -> yaclib::Future<double> {
       *      throw std::runtime_error{""};
       *    })
       *    .ThenInline([](yaclib::util::Result<double>) { // need double Value
       *      return 1;
       *    });
       */
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::error_code>) {
      if (state == util::ResultState::Error) {
        return WrapperSubscribe(std::move(r).Error());
      }
      return std::move(_promise).Set(std::move(r));
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::exception_ptr>) {
      if (state == util::ResultState::Exception) {
        return WrapperSubscribe(std::move(r).Exception());
      }
      return std::move(_promise).Set(std::move(r));
    }
  }

  Promise<U> _promise;
  FunctorStoreT _f;
};

template <bool Inline, typename Ret, typename Arg, typename Functor>
auto SetSyncCallback(FutureCorePtr<Arg> caller, Functor&& f) {
  using InvokeT = detail::SyncInvoke<Ret, decltype(std::forward<Functor>(f)), Arg>;
  using CoreT = detail::Core<Ret, InvokeT, Arg>;
  util::Ptr callback{new util::Counter<CoreT>{std::forward<Functor>(f)}};
  callback->SetExecutor(caller->GetExecutor());
  if constexpr (Inline) {
    caller->SetCallbackInline(callback);
  } else {
    caller->SetCallback(callback);
  }
  return callback;
}

template <bool Inline, typename Ret, typename Arg, typename Functor>
auto SetAsyncCallback(FutureCorePtr<Arg> caller, Functor&& f) {
  util::Ptr next_callback{new util::Counter<detail::ResultCore<Ret>>{}};
  next_callback->SetExecutor(caller->GetExecutor());
  using InvokeT = detail::AsyncInvoke<Ret, decltype(std::forward<Functor>(f)), Arg>;
  using CoreT = detail::Core<void, InvokeT, Arg>;
  util::Ptr prev_callback{new util::Counter<CoreT>{next_callback, std::forward<Functor>(f)}};
  prev_callback->SetExecutor(caller->GetExecutor());
  if constexpr (Inline) {
    caller->SetCallbackInline(prev_callback);
  } else {
    caller->SetCallback(prev_callback);
  }
  return next_callback;
}

template <bool Subscribe, bool Inline, typename Arg, typename Functor>
auto SetCallback(FutureCorePtr<Arg> caller, Functor&& f) {
  // TODO(kononovk/MBkkt): think about how to distinguish first and second overloads,
  //  when T is constructable from Result
  using AsyncRet = util::detail::ResultValueT<typename detail::Return<Arg, Functor>::Type>;
  constexpr bool kAsync = util::IsFutureV<AsyncRet>;
  using Ret = util::detail::ResultValueT<util::detail::FutureValueT<AsyncRet>>;
  if constexpr (Subscribe) {
    if constexpr (kAsync) {
      SetAsyncCallback<Inline, Ret>(std::move(caller), std::forward<Functor>(f));
    } else {
      SetSyncCallback<Inline, Ret>(std::move(caller), std::forward<Functor>(f));
    }
  } else {
    if constexpr (kAsync) {
      return Future<Ret>{SetAsyncCallback<Inline, Ret>(std::move(caller), std::forward<Functor>(f))};
    } else {
      return Future<Ret>{SetSyncCallback<Inline, Ret>(std::move(caller), std::forward<Functor>(f))};
    }
  }
}

}  // namespace detail

template <typename T>
Future<T>::~Future() {
  if (_core) {
    _core->SetState(detail::BaseCore::State::HasStop);
  }
}

template <typename T>
bool Future<T>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename T>
bool Future<T>::Ready() const& noexcept {
  return _core->GetState() == detail::BaseCore::State::HasResult;
}

template <typename T>
const util::Result<T>* Future<T>::Get() const& noexcept {
  if (/*TODO(MBkkt): Maybe we want likely*/ Ready()) {
    return &_core->Get();
  }
  return nullptr;
}

template <typename T>
util::Result<T> Future<T>::Get() && noexcept {
  if (!Ready()) {
    Wait(*this);
  }
  auto core = std::exchange(_core, nullptr);
  return std::move(core->Get());
}

template <typename T>
void Future<T>::Stop() && {
  _core->SetState(detail::BaseCore::State::HasStop);
  _core = nullptr;
}

template <typename T>
void Future<T>::Detach() && {
  _core = nullptr;
}

template <typename T>
Future<T>& Future<T>::Via(IExecutorPtr e) & {
  assert(e);
  _core->SetExecutor(std::move(e));
  return *this;
}

template <typename T>
Future<T>&& Future<T>::Via(IExecutorPtr e) && {
  return std::move(Via(e));
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(Functor&& f) && {
  return detail::SetCallback<false, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename T>
template <typename Functor>
auto Future<T>::ThenInline(Functor&& f) && {
  return detail::SetCallback<false, true>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(IExecutorPtr e, Functor&& f) && {
  assert(e);
  assert(e->Tag() != IExecutor::Type::Inline);
  _core->SetExecutor(std::move(e));
  return detail::SetCallback<false, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename T>
template <typename Functor>
void Future<T>::Subscribe(Functor&& f) && {
  detail::SetCallback<true, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename T>
template <typename Functor>
void Future<T>::SubscribeInline(Functor&& f) && {
  detail::SetCallback<true, true>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename T>
template <typename Functor>
void Future<T>::Subscribe(IExecutorPtr e, Functor&& f) && {
  assert(e);
  assert(e->Tag() != IExecutor::Type::Inline);
  _core->SetExecutor(std::move(e));
  detail::SetCallback<true, false>(std::exchange(_core, nullptr), std::forward<Functor>(f));
}

template <typename T>
Future<T>::Future(detail::FutureCorePtr<T> core) : _core{std::move(core)} {
}

template <typename T>
const detail::FutureCorePtr<T>& Future<T>::GetCore() const {
  return _core;
}

}  // namespace yaclib

#pragma once

#ifndef YACLIB_ASYNC_IMPL
#error "You can not include this header directly, use yaclib/async/async.hpp"
#endif

#include <yaclib/util/defer.hpp>

namespace yaclib::async {
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
  using Type = util::detail::ResultValueT<util::InvokeT<Functor, util::Result<T>>>;
  static constexpr bool kIsAsync = util::IsFutureV<Type>;
};

template <typename T, typename Functor>
struct Return<T, Functor, 2> {
  using Type = util::detail::ResultValueT<util::InvokeT<Functor, T>>;
  static constexpr bool kIsAsync = util::IsFutureV<Type>;
};

template <typename T, typename Functor>
struct Return<T, Functor, 4> {
  using Type = util::detail::ResultValueT<util::InvokeT<Functor, std::error_code>>;
  static constexpr bool kIsAsync = util::IsFutureV<Type>;
};

template <typename T, typename Functor>
struct Return<T, Functor, 8> {
  using Type = util::detail::ResultValueT<util::InvokeT<Functor, std::exception_ptr>>;
  static constexpr bool kIsAsync = util::IsFutureV<Type>;
};

template <typename U, typename FunctorT, typename T>
class Invoke {
  using FunctorStoreT = std::decay_t<FunctorT>;
  using FunctorInvokeT =
      std::conditional_t<std::is_function_v<std::remove_reference_t<FunctorT>>, FunctorStoreT, FunctorT>;

 public:
  explicit Invoke(FunctorStoreT&& f) noexcept(std::is_nothrow_move_constructible_v<FunctorStoreT>) : _f{std::move(f)} {
  }

  explicit Invoke(const FunctorStoreT& f) noexcept(std::is_nothrow_copy_constructible_v<FunctorStoreT>) : _f{f} {
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
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::error_code>) {
      if (state == util::ResultState::Error) {
        return {std::forward<FunctorInvokeT>(_f)(std::move(r).Error())};
      }
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::exception_ptr>) {
      if (state == util::ResultState::Exception) {
        return {std::forward<FunctorInvokeT>(_f)(std::move(r).Exception())};
      }
    }
    if constexpr (std::is_same_v<T, U>) {
      return std::move(r);
    } else {
      switch (state) {
        case util::ResultState::Error:
          return {std::move(r).Error()};
        case util::ResultState::Exception:
          return {std::move(r).Exception()};
        default:
          assert(false);
      }
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
  explicit AsyncInvoke(executor::IExecutorPtr executor, Promise<U> promise,
                       FunctorStoreT&& f) noexcept(std::is_nothrow_move_constructible_v<FunctorStoreT>)
      : _executor{std::move(executor)}, _promise{std::move(promise)}, _f{std::move(f)} {
  }

  explicit AsyncInvoke(executor::IExecutorPtr executor, Promise<U> promise,
                       const FunctorStoreT& f) noexcept(std::is_nothrow_copy_constructible_v<FunctorStoreT>)
      : _executor{std::move(executor)}, _promise{std::move(promise)}, _f{f} {
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
    std::forward<FunctorInvokeT>(_f)(std::move(a))
        .Subscribe(std::move(_executor), [promise = std::move(_promise)](util::Result<U>&& r) mutable {
          std::move(promise).Set(std::move(r));
        });
  }

  void WrapperOther(util::Result<T>&& r) {
    const auto state = r.State();
    if constexpr (util::IsInvocableV<FunctorInvokeT, T>) {
      if (state == util::ResultState::Value) {
        return WrapperSubscribe(std::move(r).Value());
      }
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::error_code>) {
      if (state == util::ResultState::Error) {
        return WrapperSubscribe(std::move(r).Error());
      }
    } else if constexpr (util::IsInvocableV<FunctorInvokeT, std::exception_ptr>) {
      if (state == util::ResultState::Exception) {
        return WrapperSubscribe(std::move(r).Exception());
      }
    }
    if constexpr (std::is_same_v<T, U>) {
      return std::move(_promise).Set(std::move(r));
    } else {
      switch (state) {
        case util::ResultState::Error:
          return std::move(_promise).Set(std::move(r).Error());
        case util::ResultState::Exception:
          return std::move(_promise).Set(std::move(r).Exception());
        default:
          assert(false);
      }
    }
  }

  executor::IExecutorPtr _executor;
  Promise<U> _promise;
  FunctorStoreT _f;
};

template <typename U, typename T, typename Functor>
Future<U> Then(FutureCorePtr<T> caller, Functor&& f) {
  using InvokeT = detail::Invoke<U, decltype(std::forward<Functor>(f)), T>;
  using CoreType = detail::Core<U, InvokeT, T>;
  util::Ptr callback{new util::Counter<CoreType>{std::forward<Functor>(f)}};

  callback->SetExecutor(caller->GetExecutor());
  caller->SetCallback(callback);

  return Future<U>{std::move(callback)};
}

template <typename U, typename T, typename Functor>
Future<U> AsyncThen(FutureCorePtr<T> caller, Functor&& f) {
  auto [future, promise] = async::MakeContract<U>();
  future.Via(caller->GetExecutor());

  using InvokeT = detail::AsyncInvoke<U, decltype(std::forward<Functor>(f)), T>;
  using CoreType = detail::Core<void, InvokeT, T>;
  util::Ptr callback{new util::Counter<CoreType>{caller->GetExecutor(), std::move(promise), std::forward<Functor>(f)}};

  callback->SetExecutor(caller->GetExecutor());
  caller->SetCallback(callback);

  return std::move(future);
}

enum class WaitPolicy {
  Endless,
  For,
  Until,
};

template <WaitPolicy kPolicy, typename T, typename Time>
bool Wait(detail::FutureCorePtr<T>& core, const Time& time) {
  if (core->Ready()) {
    return true;
  }
  util::Counter<detail::WaitCore, detail::WaitCoreDeleter> callback;
  if (core->SetWaitCallback(&callback)) {
    return true;
  }
  std::unique_lock guard{callback.m};
  if constexpr (kPolicy != WaitPolicy::Endless) {
    const bool ready = [&] {
      if constexpr (kPolicy == WaitPolicy::For) {
        return callback.cv.wait_for(guard, time, [&] {
          return callback.is_ready;
        });
      } else if constexpr (kPolicy == WaitPolicy::Until) {
        return callback.cv.wait_until(guard, time, [&] {
          return callback.is_ready;
        });
      }
    }();
    if (ready) {
      return true;
    }
    if (core->ResetAfterTimeout()) {
      return false;
    }
    // We know we have Result, but we must wait until callback was not used by executor
  }
  while (!callback.is_ready) {
    callback.cv.wait(guard);
  }
  return true;
}

}  // namespace detail

template <typename T>
Future<T>::~Future() {
  std::move(*this).Stop();
}

template <typename T>
bool Future<T>::Valid() const& noexcept {
  return _core != nullptr;
}

template <typename T>
bool Future<T>::Ready() const& noexcept {
  return _core->Ready();
}

template <typename T>
void Future<T>::Wait() & {
  detail::Wait<detail::WaitPolicy::Endless>(_core, /* stub value */ false);
}

template <typename T>
template <typename Clock, typename Duration>
bool Future<T>::WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time) & {
  return detail::Wait<detail::WaitPolicy::Until>(_core, timeout_time);
}

template <typename T>
template <typename Rep, typename Period>
bool Future<T>::WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration) & {
  return detail::Wait<detail::WaitPolicy::For>(_core, timeout_duration);
}

template <typename T>
util::Result<T> Future<T>::Get() const& {
  if (_core->Ready()) {
    return _core->Get();
  }
  return {};
}

template <typename T>
util::Result<T> Future<T>::Get() && {
  Wait();
  auto core = std::exchange(_core, nullptr);
  return std::move(core->Get());
}

template <typename T>
void Future<T>::Stop() && {
  if (auto core = std::exchange(_core, nullptr)) {
    core->Stop();
  }
}

template <typename T>
void Future<T>::Detach() && {
  _core = nullptr;
}

template <typename T>
Future<T>& Future<T>::Via(executor::IExecutorPtr executor) & {
  _core->SetExecutor(std::move(executor));
  return *this;
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(Functor&& functor) && {
  // TODO(kononovk/MBkkt): think about how to distinguish first and second overloads,
  //  when T is constructable from Result
  using Ret = detail::Return<T, Functor>;
  if constexpr (Ret::kIsAsync) {
    return detail::AsyncThen<util::detail::FutureValueT<typename Ret::Type>>(std::exchange(_core, nullptr),
                                                                             std::forward<Functor>(functor));
  } else {
    return detail::Then<util::detail::ResultValueT<typename Ret::Type>>(std::exchange(_core, nullptr),
                                                                        std::forward<Functor>(functor));
  }
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(executor::IExecutorPtr executor, Functor&& functor) && {
  _core->SetExecutor(std::move(executor));
  return std::move(*this).Then(std::forward<Functor>(functor));
}

template <typename T>
template <typename Functor>
void Future<T>::Subscribe(Functor&& functor) && {
  std::move(*this).Then(std::forward<Functor>(functor)).Detach();  // Detach Future to avoid call Stop in dtor
}

template <typename T>
template <typename Functor>
void Future<T>::Subscribe(executor::IExecutorPtr executor, Functor&& functor) && {
  _core->SetExecutor(std::move(executor));
  std::move(*this).Subscribe(std::forward<Functor>(functor));
}

template <typename T>
Future<T>::Future(detail::FutureCorePtr<T> core) : _core{std::move(core)} {
}

}  // namespace yaclib::async

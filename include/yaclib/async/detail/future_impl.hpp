#pragma once

#ifndef ASYNC_IMPL
#error "You can not include this header directly, use yaclib/async/async.hpp"
#endif

#include <condition_variable>
#include <mutex>

namespace yaclib::async {
namespace detail {

template <typename U>
struct FutureValue;

template <typename U>
struct FutureValue<Future<U>> {
  using type = U;
};

template <typename T>
using FutureValueT = typename FutureValue<T>::type;

struct Getter : ITask {
  void Call() noexcept final {
  }
};

class GetterDeleter {
 public:
  template <typename Type>
  void Delete(void*) {
    std::lock_guard guard{m};
    is_ready = true;
    // Notify under mutex, because cv located on stack memory of other thread
    cv.notify_all();
  }

  bool is_ready{false};
  std::mutex m;
  std::condition_variable cv;
};

template <typename U, typename Value>
Future<U> MakeFuture(Value val) {
  auto [f, p] = async::MakeContract<U>();
  std::move(p).Set(std::move(val));
  return std::move(f);
}

}  // namespace detail

template <typename T>
Future<T>::Future(FutureCorePtr<T> core) : _core{std::move(core)} {
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(executor::IExecutorPtr executor, Functor&& functor) && {
  // TODO(kononovk/MBkkt): think about how to distinguish first and second overloads,
  //  when T is constructable from Result
  if constexpr (util::IsInvocableV<Functor, util::Result<T>>) {
    using Ret = util::detail::ResultValueT<util::InvokeT<Functor, util::Result<T>>>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenResult<detail::FutureValueT<Ret>>(std::move(executor), std::forward<Functor>(functor));
    } else {
      return ThenResult<Ret>(std::move(executor), std::forward<Functor>(functor));
    }
  } else if constexpr (util::IsInvocableV<Functor, T>) {
    using Ret = util::detail::ResultValueT<util::InvokeT<Functor, T>>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenValue<detail::FutureValueT<Ret>>(std::move(executor), std::forward<Functor>(functor));
    } else {
      return ThenValue<Ret>(std::move(executor), std::forward<Functor>(functor));
    }
  } else if constexpr (util::IsInvocableV<Functor, std::error_code>) {
    using Ret = util::InvokeT<Functor, std::error_code>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenError(std::move(executor), std::forward<Functor>(functor));
    } else {
      return ThenError(std::move(executor), std::forward<Functor>(functor));
    }
  } else if constexpr (util::IsInvocableV<Functor, std::exception_ptr>) {
    using Ret = util::InvokeT<Functor, std::exception_ptr>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenException(std::move(executor), std::forward<Functor>(functor));
    } else {
      return ThenException(std::move(executor), std::forward<Functor>(functor));
    }
  }
  static_assert(util::IsInvocableV<Functor, util::Result<T>> || util::IsInvocableV<Functor, T> ||
                    util::IsInvocableV<Functor, std::error_code> || util::IsInvocableV<Functor, std::exception_ptr>,
                "Message from Then");  // TODO(Ri7ay): create more informative message
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(Functor&& functor) && {
  return std::move(*this).Then(_core->GetExecutor(), std::forward<Functor>(functor));
}

template <typename T>
/* Functor must return void type */
template <typename Functor>
void Future<T>::Subscribe(executor::IExecutorPtr executor, Functor&& functor) && {
  auto future = std::move(*this).Then(std::move(executor), std::forward<Functor>(functor));
  future._core = nullptr;  // Detach future to avoid Cancel() in dtor
}

template <typename T>
template <typename Functor>
void Future<T>::Subscribe(Functor&& functor) && {
  std::move(*this).Subscribe(_core->GetExecutor(), std::forward<Functor>(functor));
}

template <typename T>
Future<T>::~Future() {
  std::move(*this).Cancel();
}

template <typename T>
void Future<T>::Cancel() && {
  auto core = std::exchange(_core, nullptr);
  if (core) {
    core->Cancel();
  }
}

template <typename T>
util::Result<T> Future<T>::Get() && {
  auto core = std::exchange(_core, nullptr);

  if (!core->IsReady()) {
    container::Counter<detail::Getter, detail::GetterDeleter> getter;
    core->SetCallback(&getter);
    std::unique_lock guard{getter.m};
    while (!getter.is_ready) {
      getter.cv.wait(guard);
    }
  }
  return std::move(core->Get());
}

template <typename T>
bool Future<T>::IsReady() const noexcept {
  return _core->IsReady();
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::ThenResult(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<T> arg) mutable -> util::Result<U> {
    try {
      if constexpr (std::is_void_v<U> && !util::IsResultV<util::InvokeT<Functor, util::Result<T>>>) {
        functor(std::move(arg));
        return util::Result<U>::Default();
      } else {
        return {functor(std::move(arg))};
      }
    } catch (...) {
      return {std::current_exception()};
    }
  };

  using CoreType = Core<U, std::decay_t<decltype(wrapper)>, T>;
  container::intrusive::Ptr shared_core{new container::Counter<CoreType>{std::move(wrapper)}};
  shared_core->SetExecutor(executor);
  shared_core->SetCaller(_core);
  _core->SetExecutor(executor);
  _core->SetCallback(shared_core);
  _core = nullptr;
  return Future<U>{std::move(shared_core)};
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::ThenValue(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<T> arg) mutable -> util::Result<U> {
    switch (arg.State()) {
      case util::ResultState::Value: {
        if constexpr (std::is_void_v<U> && !util::IsResultV<util::InvokeT<Functor, T>>) {
          if constexpr (std::is_void_v<T>) {
            functor();
          } else {
            functor(std::move(arg).Value());
          }
          return util::Result<U>::Default();
        } else if constexpr (std::is_void_v<T>) {
          return {functor()};
        } else {
          return {functor(std::move(arg).Value())};
        }
      }
      case util::ResultState::Error: {
        return {std::move(arg).Error()};
      }
      case util::ResultState::Exception: {
        return {std::move(arg).Exception()};
      }
      case util::ResultState::Empty: {
        assert(false);
      }
    }
    std::terminate();
  };
  return ThenResult<U>(std::move(executor), std::move(wrapper));
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::ThenError(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<T> arg) mutable -> util::Result<T> {
    switch (arg.State()) {
      case util::ResultState::Exception:
      case util::ResultState::Value:
        return arg;
      case util::ResultState::Error: {
        return {functor(std::move(arg).Error())};
      }
      case util::ResultState::Empty: {
        assert(false);
      }
    }
    std::terminate();
  };
  return ThenResult<T>(std::move(executor), std::move(wrapper));
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::ThenException(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<T> arg) mutable -> util::Result<T> {
    switch (arg.State()) {
      case util::ResultState::Error:
      case util::ResultState::Value:
        return arg;
      case util::ResultState::Exception: {
        return {functor(std::move(arg).Exception())};
      }
      case util::ResultState::Empty: {
        assert(false);
      }
    }
    std::terminate();
  };
  return ThenResult<T>(std::move(executor), std::move(wrapper));
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::AsyncThenResult(executor::IExecutorPtr executor, Functor&& functor) {
  auto [future, promise] = async::MakeContract<U>();
  future._core->SetExecutor(executor);
  std::move(*this).Subscribe(executor, [p = std::move(promise), f = std::forward<Functor>(functor),
                                        e = executor](util::Result<T> result) mutable {
    try {
      f(std::move(result)).Subscribe(e, [p = std::move(p)](util::Result<U> res) mutable {
        std::move(p).Set(std::move(res));
      });
    } catch (...) {
      std::move(p).Set(std::current_exception());
    }
  });
  return std::move(future);
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::AsyncThenValue(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [f = std::forward<Functor>(functor)](util::Result<T> arg) {
    switch (arg.State()) {
      case util::ResultState::Value:
        return f(std::move(arg).Value());
      case util::ResultState::Error:
        return detail::MakeFuture<U, std::error_code>(std::move(arg).Error());
      case util::ResultState::Exception: {
        return detail::MakeFuture<U, std::exception_ptr>(std::move(arg).Exception());
      }
      case util::ResultState::Empty: {
        assert(false);
      }
    }
    std::terminate();
  };
  return AsyncThenResult<U>(std::move(executor), std::move(wrapper));
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::AsyncThenError(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<T> arg) mutable -> Future<util::Result<T>> {
    switch (arg.State()) {
      case util::ResultState::Value: {
        return detail::MakeFuture<T, T>(std::move(arg).Value());
      }
      case util::ResultState::Error:
        return functor(std::move(arg).Error());
      case util::ResultState::Exception:
        return detail::MakeFuture<T, std::exception_ptr>(std::move(arg).Exception());
      case util::ResultState::Empty: {
        assert(false);
      }
    }
    std::terminate();
  };
  return AsyncThenResult<T>(std::move(executor), std::move(wrapper));
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::AsyncThenException(executor::IExecutorPtr executor, Functor&& functor) {
  auto wrapper = [functor = std::forward<Functor>(functor)](util::Result<T> arg) mutable -> Future<util::Result<T>> {
    switch (arg.State()) {
      case util::ResultState::Value: {
        return detail::MakeFuture<T, T>(std::move(arg).Value());
      }
      case util::ResultState::Error:
        return detail::MakeFuture<T, std::exception_ptr>(std::move(arg).Error());
      case util::ResultState::Exception:
        return functor(std::move(arg).Exception());
      case util::ResultState::Empty: {
        assert(false);
      }
    }
    std::terminate();
  };
  return AsyncThenResult<T>(std::move(executor), std::move(wrapper));
}

}  // namespace yaclib::async

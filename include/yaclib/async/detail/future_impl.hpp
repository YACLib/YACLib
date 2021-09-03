#pragma once

#ifndef ASYNC_IMPL
#error "You can not include this header directly, use yaclib/async/async.hpp"
#endif

#include <condition_variable>
#include <mutex>

namespace yaclib::async {
namespace detail {

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
Future<T>::Future(detail::FutureCorePtr<T> core) : _core{std::move(core)} {
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(Functor&& functor) && {
  // TODO(kononovk/MBkkt): think about how to distinguish first and second overloads,
  //  when T is constructable from Result
  if constexpr (util::IsInvocableV<Functor, util::Result<T>>) {
    using Ret = util::detail::ResultValueT<util::InvokeT<Functor, util::Result<T>>>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenResult<util::detail::FutureValueT<Ret>>(std::forward<Functor>(functor));
    } else {
      return ThenResult<Ret>(std::forward<Functor>(functor));
    }
  } else if constexpr (util::IsInvocableV<Functor, T>) {
    using Ret = util::detail::ResultValueT<util::InvokeT<Functor, T>>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenValue<util::detail::FutureValueT<Ret>>(std::forward<Functor>(functor));
    } else {
      return ThenValue<Ret>(std::forward<Functor>(functor));
    }
  } else if constexpr (util::IsInvocableV<Functor, std::error_code>) {
    using Ret = util::InvokeT<Functor, std::error_code>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenError(std::forward<Functor>(functor));
    } else {
      return ThenError(std::forward<Functor>(functor));
    }
  } else if constexpr (util::IsInvocableV<Functor, std::exception_ptr>) {
    using Ret = util::InvokeT<Functor, std::exception_ptr>;
    if constexpr (util::IsFutureV<Ret>) {
      return AsyncThenException(std::forward<Functor>(functor));
    } else {
      return ThenException(std::forward<Functor>(functor));
    }
  }
  static_assert(util::IsInvocableV<Functor, util::Result<T>> || util::IsInvocableV<Functor, T> ||
                    util::IsInvocableV<Functor, std::error_code> || util::IsInvocableV<Functor, std::exception_ptr>,
                "Message from Then");  // TODO(Ri7ay): create more informative message
}

template <typename T>
template <typename Functor>
auto Future<T>::Then(executor::IExecutorPtr executor, Functor&& functor) && {
  _core->SetExecutor(std::move(executor));
  return std::move(*this).Then(std::forward<Functor>(functor));
}

template <typename T>
/* Functor must return void type */
template <typename Functor>
void Future<T>::Subscribe(Functor&& functor) && {
  auto future = std::move(*this).Then(std::forward<Functor>(functor));
  future._core = nullptr;  // Detach future to avoid Cancel() in dtor
}

template <typename T>
template <typename Functor>
void Future<T>::Subscribe(executor::IExecutorPtr executor, Functor&& functor) && {
  _core->SetExecutor(std::move(executor));
  std::move(*this).Subscribe(std::forward<Functor>(functor));
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
Future<U> Future<T>::ThenResult(Functor&& f) {
  auto wrapper = [f = std::forward<Functor>(f)](util::Result<T> r) mutable -> util::Result<U> {
    try {
      if constexpr (std::is_void_v<U> && !util::IsResultV<util::InvokeT<Functor, util::Result<T>>>) {
        std::forward<Functor>(f)(std::move(r));
        return util::Result<U>::Default();
      } else {
        if constexpr (std::is_function_v<std::remove_reference_t<Functor>>) {
          return {(f)(std::move(r).Value())};
        } else {
          return {std::forward<Functor>(f)(std::move(r))};
        }
      }
    } catch (...) {
      return {std::current_exception()};
    }
  };
  using CoreType = detail::Core<U, std::decay_t<decltype(wrapper)>, T>;
  container::intrusive::Ptr core{new container::Counter<CoreType>{std::move(wrapper)}};
  core->SetExecutor(_core->GetExecutor());
  core->SetCaller(_core);
  _core->SetCallback(core);
  _core = nullptr;
  return Future<U>{std::move(core)};
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::ThenValue(Functor&& f) {
  return ThenResult<U>([f = std::forward<Functor>(f)](util::Result<T> r) mutable -> util::Result<U> {
    switch (r.State()) {
      case util::ResultState::Value:
        if constexpr (std::is_void_v<U> && !util::IsResultV<util::InvokeT<Functor, T>>) {
          if constexpr (std::is_void_v<T>) {
            std::forward<Functor>(f)();
          } else {
            std::forward<Functor>(f)(std::move(r).Value());
          }
          return util::Result<U>::Default();
        } else if constexpr (std::is_void_v<T>) {
          return {std::forward<Functor>(f)()};
        } else {
          // TODO: its very bad, think how to clean this code and make it general for all cases
          if constexpr (std::is_function_v<std::remove_reference_t<Functor>>) {
            return {(f)(std::move(r).Value())};
          } else {
            return {std::forward<Functor>(f)(std::move(r).Value())};
          }
        }
      case util::ResultState::Error:
        return {std::move(r).Error()};
      case util::ResultState::Exception:
        return {std::move(r).Exception()};
      case util::ResultState::Empty:
        assert(false);
        std::terminate();
    }
  });
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::ThenError(Functor&& f) {
  return ThenResult<T>([f = std::forward<Functor>(f)](util::Result<T> r) mutable -> util::Result<T> {
    switch (r.State()) {
      case util::ResultState::Value:
      case util::ResultState::Exception:
        return r;
      case util::ResultState::Error:
        return {std::forward<Functor>(f)(std::move(r).Error())};
      case util::ResultState::Empty:
        assert(false);
        std::terminate();
    }
  });
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::ThenException(Functor&& f) {
  return ThenResult<T>([f = std::forward<Functor>(f)](util::Result<T> r) mutable -> util::Result<T> {
    switch (r.State()) {
      case util::ResultState::Value:
      case util::ResultState::Error:
        return r;
      case util::ResultState::Exception:
        return {std::forward<Functor>(f)(std::move(r).Exception())};
      case util::ResultState::Empty:
        assert(false);
        std::terminate();
    }
  });
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::AsyncThenResult(Functor&& f) {
  auto [future, promise] = async::MakeContract<U>();
  future._core->SetExecutor(_core->GetExecutor());
  std::move(*this).Subscribe([f = std::forward<Functor>(f), e = _core->GetExecutor(),
                              promise = std::move(promise)](util::Result<T> r) mutable {
    try {
      std::forward<Functor>(f)(std::move(r))
          .Subscribe(std::move(e), [promise = std::move(promise)](util::Result<U> r) mutable {
            std::move(promise).Set(std::move(r));
          });
    } catch (...) {
      std::move(promise).Set(std::current_exception());
    }
  });
  return std::move(future);
}

template <typename T>
template <typename U, typename Functor>
Future<U> Future<T>::AsyncThenValue(Functor&& f) {
  return AsyncThenResult<U>([f = std::forward<Functor>(f)](util::Result<T> r) mutable {
    switch (r.State()) {
      case util::ResultState::Value:
        return std::forward<Functor>(f)(std::move(r).Value());
      case util::ResultState::Error:
        return detail::MakeFuture<U, std::error_code>(std::move(r).Error());
      case util::ResultState::Exception:
        return detail::MakeFuture<U, std::exception_ptr>(std::move(r).Exception());
      case util::ResultState::Empty:
        assert(false);
        std::terminate();
    }
  });
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::AsyncThenError(Functor&& f) {
  return AsyncThenResult<T>([f = std::forward<Functor>(f)](util::Result<T> r) mutable {
    switch (r.State()) {
      case util::ResultState::Value:
        return detail::MakeFuture<T, T>(std::move(r).Value());
      case util::ResultState::Error:
        return std::forward<Functor>(f)(std::move(r).Error());
      case util::ResultState::Exception:
        return detail::MakeFuture<T, std::exception_ptr>(std::move(r).Exception());
      case util::ResultState::Empty:
        assert(false);
        std::terminate();
    }
  });
}

template <typename T>
template <typename Functor>
Future<T> Future<T>::AsyncThenException(Functor&& f) {
  return AsyncThenResult<T>([f = std::forward<Functor>(f)](util::Result<T> r) mutable {
    switch (r.State()) {
      case util::ResultState::Value:
        return detail::MakeFuture<T, T>(std::move(r).Value());
      case util::ResultState::Error:
        return detail::MakeFuture<T, std::exception_ptr>(std::move(r).Error());
      case util::ResultState::Exception:
        return std::forward<Functor>(f)(std::move(r).Exception());
      case util::ResultState::Empty:
        assert(false);
        std::terminate();
    }
  });
}

}  // namespace yaclib::async

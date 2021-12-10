#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/lazy/util.hpp>

#include <condition_variable>
#include <iostream>
#include <optional>
#include <type_traits>

namespace yaclib::detail {

template <typename LazyProxy>
class LazyCore : public ITask {
 public:
  LazyCore(LazyProxy&& lp) : _lp(std::move(lp)) {
  }

  void Call() noexcept final {
    /*AnotherMethod(_index);
    if (_index != GetLazyProxySize<LazyProxy>::value) {
      Execute();
    }*/
  }

  void Cancel() noexcept final {
  }

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
  }

  void Execute() {
    /*if (GetProxySize<LazyProxy>::value == _index) {
      _index++;
      _e->Execute(*this);
    } else {
      _prev.Execute();
    }*/
  }

  void AnotherMethod(int) {
    /* if (GetProxySize<LazyProxy>::value == index) {
       _f();
     } else {
       _prev.AnotherMethod(index);
     }*/
  }

 private:
  LazyProxy _lp;
};

template <typename Functor, typename PrevP = Nil>
class LazyProxy /*: public ITask*/ {
 public:
  using PrevProxy = PrevP;
  using ReturnType = util::InvokeT<Functor, typename PrevProxy::ReturnType>;
  using FunctorStoreT = std::decay_t<Functor>;

  template <typename T>
  friend class LazyCore;

  LazyProxy(IExecutorPtr e, FunctorStoreT&& f, PrevProxy&& prev) : _e(e), _f(std::move(f)), _prev(std::move(prev)) {
  }

  LazyProxy(IExecutorPtr e, const FunctorStoreT& f, const PrevProxy& prev) : _f(f), _e(e), _prev(prev) {
  }

  // Constructor for base LazyProxy object only
  LazyProxy(IExecutorPtr e, FunctorStoreT&& f) : _e(std::move(e)), _f(std::move(f)), _prev(Nil{}) {
    static_assert(std::is_same_v<PrevProxy, Nil>);
  }

  template <typename F>
  auto Then(F&& f) && {
    return LazyProxy<F, LazyProxy>{_e, std::forward<F>(f), std::move(*this)};
  }

  template <typename F>
  auto Then(IExecutorPtr e, F&& f) && {
    return LazyProxy<F, LazyProxy>{std::move(e), std::forward<F>(f), std::move(*this)};
  }

  template <typename T>
  static IExecutorPtr GetFirstExecutor(T&& lazy) {
    using U = std::decay_t<T>;
    if constexpr (std::is_same_v<typename U::PrevProxy, Nil>) {
      return lazy._e;
    } else {
      return GetFirstExecutor(lazy);
    }
  }

  ReturnType Get() && {
    ReturnType result;
    std::condition_variable cv;
    std::mutex m;
    bool ready{false};
    IExecutorPtr e = _e;

    auto final = std::move(*this).Then([&](ReturnType r) {
      result = std::move(r);
      {
        std::lock_guard guard{m};
        ready = true;
      }
      cv.notify_one();
    });
    LazyCore<decltype(final)> core{std::move(final)};
    GetFirstExecutor(final)->Execute(core);

    {
      std::unique_lock guard{m};
      while (!ready) {
        cv.wait(guard);
      }
    }
    return result;
  }

 private:
  IExecutorPtr _e{MakeInline()};
  FunctorStoreT _f;
  PrevProxy _prev{};
};

template <class Functor>
LazyProxy(IExecutorPtr, Functor&&) -> LazyProxy<std::decay_t<Functor>, Nil>;

}  // namespace yaclib::detail

namespace yaclib {

template <typename Functor>
auto LazyRun(IExecutorPtr e, Functor&& f) {
  return detail::LazyProxy{std::move(e), std::forward<Functor>(f)};
}

}  // namespace yaclib

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

template <typename Lazy, typename Res>
auto ReverseLazy(Lazy& l, Res res);

template <typename T>
class LazyCore;

template <class T>
LazyCore(T&& lp) -> LazyCore<decltype(ReverseLazy(lp, Nil{}))>;

template <typename Functor, typename Arg, typename Type>
struct InvokeWrapper;

template <typename Functor, typename Arg, typename Type = util::InvokeT<Functor, Arg>>
struct InvokeWrapper {
  using type = util::InvokeT<Functor, Arg>;
};

template <typename Functor, typename Arg>
struct InvokeWrapper<Functor, Arg, void> {
  using type = void;
};

template <typename Functor, typename Arg>
using InvokeWrapperT = typename InvokeWrapper<Functor, Arg>::type;

template <typename Functor, typename PrevP = Nil, bool IsReversed = false, typename Ret = void>
class LazyProxy {
 public:
  using PrevProxy = PrevP;
  using ReturnType = std::conditional_t<IsReversed, Ret, InvokeWrapperT<Functor, typename PrevProxy::ReturnType>>;
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

  ReturnType Get() && {
    ReturnType result;
    std::condition_variable cv;
    std::mutex m;
    bool ready{false};

    auto final = std::move(*this).Then([&](ReturnType r) {
      result = std::move(r);
      {
        std::lock_guard guard{m};
        ready = true;
      }
      cv.notify_one();
    });
    LazyCore<decltype(ReverseLazy(final, Nil{}))> core{std::move(final)};
    core.GetExecutor()->Execute(core);

    {
      std::unique_lock guard{m};
      while (!ready) {
        cv.wait(guard);
      }
    }
    return result;
  }

 public:
  IExecutorPtr _e{MakeInline()};
  FunctorStoreT _f;
  PrevProxy _prev{};
};

template <class Functor>
LazyProxy(IExecutorPtr, Functor&&) -> LazyProxy<std::decay_t<Functor>, Nil>;

template <typename Lazy, typename Res>
auto ReverseLazy(Lazy& l, Res res) {
  if constexpr (!std::is_same_v<Lazy, Nil>) {
    return ReverseLazy(l._prev, LazyProxy<typename Lazy::FunctorStoreT, decltype(res), true, typename Lazy::ReturnType>{
                                    std::move(l._e), std::move(l._f), std::move(res)});
  } else {
    return res;
  }
}

template <typename LazyProxy>
class LazyCore : public ITask {
 public:
  template <typename T>
  LazyCore(T&& lp) : _lp(ReverseLazy(lp, Nil{})) {
  }

  void Call() noexcept final {
    std::cout << "HERE!!!" << std::endl;
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

  IExecutorPtr GetExecutor() {
    return MakeInline();
  }

 private:
  LazyProxy _lp;
};

}  // namespace yaclib::detail

namespace yaclib {

template <typename Functor>
auto LazyRun(IExecutorPtr e, Functor&& f) {
  return detail::LazyProxy{std::move(e), std::forward<Functor>(f)};
}

}  // namespace yaclib

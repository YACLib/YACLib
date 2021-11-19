#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/executor.hpp>

#include <condition_variable>
#include <iostream>
#include <optional>
#include <type_traits>

namespace yaclib::detail {

template <typename Functor, typename PrevProxy>
class Proxy;

struct NilFunctor {};
struct Nil {
  using FunctorT = void;
  using ReturnType = void;
};

template <typename P>
struct GetProxySize;

template <>
struct GetProxySize<Nil> {
  constexpr static size_t value = 0;
};

template <typename Functor, typename PrevProxy>
struct GetProxySize<Proxy<Functor, PrevProxy>> {
  constexpr static size_t value = 1 + GetProxySize<PrevProxy>::value;
};

template <typename T, typename Proxy>
class LazyCore : ITask {};

template <typename Functor, typename PrevProxy = Nil>
class Proxy : public ITask {
 public:
  using ReturnType = util::InvokeT<Functor, typename PrevProxy::ReturnType>;
  using PrevP = PrevProxy;
  using FunctorStoreT = std::decay_t<Functor>;

  Proxy(FunctorStoreT&& f, IExecutorPtr e, PrevProxy&& prev)
      : _f(std::move(f)), _e(std::move(e)), _prev(std::move(prev)) {
  }

  Proxy(const FunctorStoreT& f, IExecutorPtr e, const PrevProxy& prev) : _f(f), _e(std::move(e)), _prev(prev) {
  }

  // Constructor for base proxy object only
  Proxy(FunctorStoreT&& f, IExecutorPtr e) : _f(std::move(f)), _e(std::move(e)), _prev(Nil{}) {
    static_assert(std::is_same_v<PrevProxy, Nil>);
  }

  void Execute() {
    if (GetProxySize<Proxy>::value == _index) {
      _index++;
      _e->Execute(*this);
    } else {
      _prev.Execute();
    }
  }

  void AnotherMethod(int index) {
    if (GetProxySize<Proxy>::value == index) {
      _f();
    } else {
      _prev.AnotherMethod(index);
    }
  }

  void Call() noexcept final {
    AnotherMethod(_index);
    if (_index != GetProxySize<Proxy>::value) {
      Execute();
    }
  }

  template <typename F>
  auto Then(F&& f) && {
    return Proxy<F, Proxy>{std::forward<F>(f), _e, std::move(*this)};
  }

  template <typename F>
  auto Then(IExecutorPtr e, F&& f) && {
    return Proxy<F, Proxy>{std::forward<F>(f), e, std::move(*this)};
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
    LazyCore<ReturnType, Proxy> core{final};
    _e->Execute(core);

    std::unique_lock guard{m};
    while (!ready) {
      cv.wait(guard);
    }
    guard.unlock();
    return result;
  }

  Future<ReturnType> Make() && {
    using CoreT = detail::LazyCore<ReturnType, Proxy>;
    IExecutorPtr e = _e;
    util::Ptr core{new util::Counter<CoreT>{std::move(*this)}};
    e->Execute(*core);
    return Future<ReturnType>{std::move(core)};
  }

 private:
  FunctorStoreT _f;
  IExecutorPtr _e;
  PrevProxy _prev{};
  size_t _index{0};
};

template <class Functor>
Proxy(Functor&&, IExecutorPtr) -> Proxy<std::decay_t<Functor>, Nil>;

}  // namespace yaclib::detail

namespace yaclib {

template <typename Functor>
auto LazyRun(IExecutorPtr e, Functor&& f) {
  return detail::Proxy{std::forward<Functor>(f), std::move(e)};
}

}  // namespace yaclib

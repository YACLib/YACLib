#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/executor.hpp>

#include <iostream>
#include <optional>
#include <type_traits>

namespace yaclib::detail {

class Callable : public ITask {
 public:
  void operator()() {
    std::cout << "Callable" << std::endl;
  }
};

template <typename Functor, typename PrevProxy>
class Proxy;

struct NilFunctor {};
struct Nil {
  using FunctorT = void;
  using ReturnType = void;
};

template <typename T, typename Proxy>
class LazyCore;

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

template <typename T, typename F, typename PP>
class LazyCore<T, Proxy<F, PP>> : public ResultCore<T> {
 public:
  using ProxyT = Proxy<F, PP>;
  static constexpr size_t kProxySize = GetProxySize<ProxyT>::value;

  LazyCore(ProxyT&& proxy) : _proxy(std::move(proxy)) {
  }

  void Call() noexcept final {
    std::cout << "Here" << std::endl;
    /*auto e = _proxy.Call(_state);
    _state++;
    e->Execute(*this);*/
  }

 private:
  ProxyT _proxy;
  size_t _state{0};
};

template <typename Functor, typename PrevProxy = Nil>
class Proxy {
  // TODO: static_assert that PrevProxy == Nil or PrevProxy == Proxy<T>
  // TODO: add overloading for const lvalue ref, etc.
 public:
  using ReturnType = util::InvokeT<Functor, typename PrevProxy::ReturnType>;
  using PrevP = PrevProxy;
  using FunctorStoreT = std::decay_t<Functor>;

  Proxy(FunctorStoreT&& f, IExecutorPtr e, PrevProxy&& prev)
      : _f(std::move(f)), _e(std::move(e)), _next(std::move(prev)) {
  }

  Proxy(const FunctorStoreT& f, IExecutorPtr e, const PrevProxy& prev) : _f(f), _e(std::move(e)), _next(prev) {
  }

  // Constructor for base proxy object only
  Proxy(FunctorStoreT&& f, IExecutorPtr e) : _f(std::move(f)), _e(std::move(e)), _next(Nil{}) {
    static_assert(std::is_same_v<PrevProxy, Nil>);
  }

  IExecutorPtr Call(int) {
    return _e;
    // iterate to i
    // variant[i + 1] = _f(variant[i]);
    // return executor[i +1];
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
  PrevProxy _next{};
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

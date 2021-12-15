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

template <typename Functor, typename PrevP = Nil>
class LazyProxy {
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

  ReturnType Get() &&;

 public:
  IExecutorPtr _e{MakeInline()};
  FunctorStoreT _f;
  PrevProxy _prev{};
};

template <typename Functor, typename PrevP = Nil, typename Ret = void>
class ReversedLazyProxy : public ITask {
 public:
  using PrevProxy = PrevP;
  using ReturnType = Ret;
  using FunctorStoreT = std::decay_t<Functor>;

  template <typename T>
  friend class LazyCore;

  void Call() noexcept final {
    /*_e->Execute([&]() {
      auto res = _f();
      _prev._e->Execute(_prev);
    });*/
  }

  void Cancel() noexcept final {
  }

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
  }

  ReversedLazyProxy(IExecutorPtr e, FunctorStoreT&& f, PrevProxy&& prev)
      : _e(e), _f(std::move(f)), _prev(std::move(prev)) {
  }

  ReversedLazyProxy(IExecutorPtr e, const FunctorStoreT& f, const PrevProxy& prev) : _f(f), _e(e), _prev(prev) {
  }

  // Constructor for base LazyProxy object only
  ReversedLazyProxy(IExecutorPtr e, FunctorStoreT&& f) : _e(std::move(e)), _f(std::move(f)), _prev(Nil{}) {
    static_assert(std::is_same_v<PrevProxy, Nil>);
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
    return ReverseLazy(l._prev,
                       ReversedLazyProxy<typename Lazy::FunctorStoreT, decltype(res), typename Lazy::ReturnType>{
                           std::move(l._e), std::move(l._f), std::move(res)});
  } else {
    return res;
  }
}

template <typename Functor, typename PrevP>
auto LazyProxy<Functor, PrevP>::Get() && -> typename LazyProxy<Functor, PrevP>::ReturnType {
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
  auto reversed_list = ReverseLazy(final, Nil{});
  LazyCore core{std::move(reversed_list)};
  core.GetExecutor()->Execute(core);

  {
    std::unique_lock guard{m};
    while (!ready) {
      cv.wait(guard);
    }
  }
  return result;
}

template <typename LazyProxy>
class LazyCore : public ITask {
 public:
  static constexpr std::size_t _size{GetProxySize<LazyProxy>::value};
  LazyCore(LazyProxy&& lp) : _lp(std::move(lp)) {
    assert(_size > 1);
  }

  void Call() noexcept final {
    // auto curr = Execute(_index);
    // _lp._e->Execute(_lp);
    /*_lp._f();
    std::cout << _size << std::endl;
    Execute(_index);
    if (_index != _size) {
      Execute();
    }*/
  }

  //  void Execute() {
  //    /*if (GetProxySize<LazyProxy>::value == _index) {
  //      _index++;
  //      _e->Execute(*this);
  //    } else {
  //      _prev.Execute();
  //    }*/
  //  }
  //
  auto Execute(std::size_t) {
    /*if (_index == _size) {
      _lp._f();
    } else {
      _prev.AnotherMethod(index);
    }*/
  }

  void Cancel() noexcept final {
  }

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
  }

  IExecutorPtr GetExecutor() {
    return _lp._e;
  }

 private:
  LazyProxy _lp;
  std::size_t _index = 0;
  typename detail::GetLazyTypes<LazyProxy>::type result;
};

}  // namespace yaclib::detail

namespace yaclib {

template <typename Functor>
auto LazyRun(IExecutorPtr e, Functor&& f) {
  return detail::LazyProxy{std::move(e), std::forward<Functor>(f)};
}

}  // namespace yaclib

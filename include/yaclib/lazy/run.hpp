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

template <typename T, typename LazyProxy>
class LazyCore : public ITask {
 public:
  LazyCore(LazyProxy&) {
  }
  void Call() noexcept final {
  }
  void Cancel() noexcept final {
  }
  void IncRef() noexcept final {
  }
  void DecRef() noexcept final {
  }
};

template <typename Functor, typename PrevLazyProxy = Nil>
class LazyProxy /*: public ITask*/ {
 public:
  using ReturnType = util::InvokeT<Functor, typename PrevLazyProxy::ReturnType>;
  using PrevP = PrevLazyProxy;
  using FunctorStoreT = std::decay_t<Functor>;

  LazyProxy(IExecutorPtr e, FunctorStoreT&& f, PrevLazyProxy&& prev)
      : _e(std::move(e)), _f(std::move(f)), _prev(std::move(prev)) {
  }

  LazyProxy(IExecutorPtr e, const FunctorStoreT& f, const PrevLazyProxy& prev) : _f(f), _e(std::move(e)), _prev(prev) {
  }

  // Constructor for base LazyProxy object only
  LazyProxy(IExecutorPtr e, FunctorStoreT&& f) : _e(std::move(e)), _f(std::move(f)), _prev(Nil{}) {
    static_assert(std::is_same_v<PrevLazyProxy, Nil>);
  }

  void Execute() {
    if (GetProxySize<LazyProxy>::value == _index) {
      _index++;
      _e->Execute(*this);
    } else {
      _prev.Execute();
    }
  }

  void AnotherMethod(int index) {
    if (GetProxySize<LazyProxy>::value == index) {
      _f();
    } else {
      _prev.AnotherMethod(index);
    }
  }

  /*void Call() noexcept final {
    AnotherMethod(_index);
    if (_index != GetLazyProxySize<LazyProxy>::value) {
      Execute();
    }
  }*/

  template <typename F>
  auto Then(F&& f) && {
    return LazyProxy<F, LazyProxy>{_e, std::forward<F>(f), std::move(*this)};
  }

  template <typename F>
  auto Then(IExecutorPtr e, F&& f) && {
    return LazyProxy<F, LazyProxy>{std::forward<F>(f), e, std::move(*this)};
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
    auto core = LazyCore<ReturnType, decltype(final)>{final};
    _e->Execute(core);

    {
      std::unique_lock guard{m};
      while (!ready) {
        cv.wait(guard);
      }
    }
    return result;
  }

  /*Future<ReturnType> Make() && {
    using CoreT = detail::LazyCore<ReturnType, LazyProxy>;
    IExecutorPtr e = _e;
    util::Ptr core{new util::Counter<CoreT>{std::move(*this)}};
    e->Execute(*core);
    return Future<ReturnType>{std::move(core)};
  }*/

 private:
  IExecutorPtr _e;
  FunctorStoreT _f;
  PrevLazyProxy _prev{};
  size_t _index{0};
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

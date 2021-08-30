#pragma once

#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/task.hpp>
#include <yaclib/util/result.hpp>

#include <atomic>
#include <cassert>
#include <type_traits>

namespace yaclib::async {

template <typename Value>
class BaseCore : public ITask {
 protected:
  enum class State {
    Empty,
    HasCallback,
    HasResult,
    Canceled,
  };

 public:
  void Cancel() {
    _state.store(State::Canceled, std::memory_order_release);
    _caller = nullptr;
  }

  void Set(util::Result<Value>&& result) {
    _result = std::move(result);
    const auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    if (state == State::HasCallback) {
      Execute();
    }
  }

  bool IsReady() const noexcept {
    return _state.load(std::memory_order_acquire) != State::Empty;
  }

  void SetCallback(container::intrusive::Ptr<ITask> callback) {
    _callback = std::move(callback);
    const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
    if (state == State::HasResult) {
      Execute();
    }
  }

  void SetExecutor(executor::IExecutorPtr executor) noexcept {
    _executor = std::move(executor);
  }

  executor::IExecutorPtr GetExecutor() const noexcept {
    return _executor;
  }

  util::Result<Value>& Get() {
    return _result;
  }

 protected:
  std::atomic<State> _state{State::Empty};
  executor::IExecutorPtr _executor{executor::MakeInlineExecutor()};
  container::intrusive::Ptr<ITask> _caller;
  container::intrusive::Ptr<ITask> _callback;

 private:
  void Execute() {
    _executor->Execute(*_callback);
    _callback = nullptr;
    _executor = nullptr;
  }

  util::Result<Value> _result;
};

template <typename Ret, typename Functor, typename Arg>
class Core : public BaseCore<Ret> {
  using Base = BaseCore<Ret>;

 public:
  explicit Core(Functor&& functor) : _functor{std::move(functor)} {
  }
  explicit Core(const Functor& functor) : _functor{functor} {
  }

  void SetCaller(container::intrusive::Ptr<BaseCore<Arg>> caller) {
    Base::_caller = std::move(caller);
  }

 private:
  void Call() noexcept final {
    auto ret = [&] {
      auto* caller = static_cast<BaseCore<Arg>*>(Base::_caller.Get());
      if constexpr (std::is_void_v<Arg>) {
        if (!caller) {
          return util::Result<Arg>::Default();
        }
      }
      return std::move(caller->Get());
    }();

    Base::_caller = nullptr;
    Base::Set(_functor(std::move(ret)));
  }

 private:
  Functor _functor;
};

template <typename Result>
class Core<Result, void, void> : public BaseCore<Result> {
  void Call() noexcept final {
    assert(false);  // this class using only via promise
  }
};

template <typename Value>
using PromiseCorePtr = container::intrusive::Ptr<Core<Value, void, void>>;

template <typename Value>
using FutureCorePtr = container::intrusive::Ptr<BaseCore<Value>>;

}  // namespace yaclib::async

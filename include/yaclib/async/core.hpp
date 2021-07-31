#pragma once

#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/task.hpp>

#include <atomic>
#include <iostream>
#include <type_traits>

namespace yaclib::async {

class BaseCore : public ITask {
 public:
  bool HasResult() const noexcept {
    return _state.load(std::memory_order_acquire) == State::HasResult;
  }

  void SetCallback(ITask& callback) {
    callback.IncRef();
    _callback = &callback;
    auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
    if (state == State::HasResult) {
      Execute();
    }
  }
  /*virtual BaseCore* GetPrev(){
    re
  }*/
  void ResetCallback() {
    if (_callback) {
      _callback->DecRef();
      _callback = nullptr;
    }
  }

  void Cancel() noexcept {
    _state.store(State::Canceled, std::memory_order_release);
  }

  void SetExecutor(executor::IExecutorPtr executor) {
    _executor = executor;
  }

  executor::IExecutorPtr GetExecutor() const {
    return _executor;
  }
  ~BaseCore() override {
    ResetCallback();
  }

 protected:
  enum class State {
    None,
    HasCallback,
    HasResult,
    Canceled,
  };

  void Execute() {
    _executor->Execute(*_callback);
    _callback->DecRef();
    _callback = nullptr;
  }

  std::atomic<State> _state{State::None};
  ITask* _callback;
  executor::IExecutorPtr _executor{executor::MakeInlineExecutor()};
};

template <typename Result>
class ResultCore : public BaseCore {
 public:
  Result GetResult() {
    return std::move(*reinterpret_cast<Result*>(&_result));
  }

  void SetResult(Result&& res) {
    new (&_result) Result{std::move(res)};
    auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    if (state == State::HasCallback) {
      Execute();
    }
  }

  void SetResult(const Result& res) {
    new (&_result) Result{res};
    auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    if (state == State::HasCallback) {
      Execute();
    }
  }

 protected:
  std::aligned_storage_t<sizeof(Result), alignof(Result)> _result;
};

template <>
class ResultCore<void> : public BaseCore {};

template <typename Result, typename Functor, typename Arg>
class Core : public ResultCore<Result> {
  using Base = ResultCore<Result>;
  using Base::_executor;
  using Base::_state;

 public:
  Core(Functor&& functor) : ResultCore<Result>{}, _functor{std::move(functor)} {
  }

  Core(const Functor& functor) : _functor(functor) {
  }

  void Call() noexcept final {
    auto arg = _prev->GetResult();
    _prev = nullptr;

    if constexpr (!std::is_void_v<Result>) {
      Base::SetResult(_functor(std::move(arg)));
    } else {
      _functor(std::move(arg));
    }

    auto state =
        _state.exchange(Base::State::HasResult, std::memory_order_acq_rel);
    if (state == Base::State::HasCallback) {
      Base::Execute();
    }
  }

  void SetPrev(container::intrusive::Ptr<ResultCore<Arg>> prev_state) {
    _prev = std::move(prev_state);
  }

 private:
  Functor _functor;
  container::intrusive::Ptr<ResultCore<Arg>> _prev;
};

template <typename Result, typename Functor>
class Core<Result, Functor, void> : public ResultCore<Result> {
  using Base = ResultCore<Result>;
  using Base::_executor;
  using Base::_state;

 public:
  Core(Functor&& functor) : _functor{std::move(functor)} {
  }
  Core(const Functor& functor) : _functor(functor) {
  }

  void Call() noexcept final {
    if constexpr (!std::is_void_v<Result>) {
      Base::SetResult(_functor());
    } else {
      _functor();
    }

    auto state =
        _state.exchange(Base::State::HasResult, std::memory_order_acq_rel);
    if (state == Base::State::HasCallback) {
      Base::Execute();
    }
  }

 private:
  Functor _functor;
};

template <typename Result>
class Core<Result, void, void> : public ResultCore<Result> {
  using Base = ResultCore<Result>;
  using Base::_executor;
  using Base::_state;

 public:
  void Call() noexcept final {
  }
};

template <typename T>
using PromiseCorePtr = container::intrusive::Ptr<Core<T, void, void>>;

template <typename Result>
using FutureCorePtr = container::intrusive::Ptr<ResultCore<Result>>;

}  // namespace yaclib::async

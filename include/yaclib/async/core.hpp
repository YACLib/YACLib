#pragma once

#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/task.hpp>

#include <atomic>
#include <iostream>
#include <type_traits>
#include <cassert>

namespace yaclib::async {

class BaseCore : public ITask {
 public:
  bool HasResult() const noexcept {
    return _state.load(std::memory_order_acquire) == State::HasResult;
  }

  void SetCallback(ITask& callback) {
    _callback = &callback;
    auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
    if (state == State::HasResult) {
      Execute();
    }
  }

  void Cancel() noexcept {
    assert(_callback == nullptr);
    _state.store(State::Canceled, std::memory_order_release);
    ResetPrev();
  }

  void SetExecutor(executor::IExecutorPtr executor) noexcept {
    _executor = std::move(executor);
  }

  executor::IExecutorPtr GetExecutor() const noexcept {
    return _executor;
  }

  void SetResult() {
    auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    if (state == State::HasCallback) {
      Execute();
    }
  }

 protected:
  enum class State {
    None,
    HasCallback,
    HasResult,
    Canceled,
  };

  std::atomic<State> _state{State::None};
  container::intrusive::Ptr<ITask> _callback;
  executor::IExecutorPtr _executor{executor::MakeInlineExecutor()};

 private:
  void Execute() {
    _executor->Execute(*_callback);
    _callback = nullptr;
  }

  virtual void ResetPrev() {}
};

template <typename Result>
class ResultCore : public BaseCore {
 public:
  Result GetResult() {
    return std::move(*reinterpret_cast<Result*>(&_result));
  }

  void SetResult(Result&& result) {
    new (&_result) Result{std::move(result)};
    BaseCore::SetResult();
  }

  void SetResult(const Result& result) {
    new (&_result) Result{result};
    BaseCore::SetResult();
  }

 private:
  std::aligned_storage_t<sizeof(Result), alignof(Result)> _result;
};

template <>
class ResultCore<void> : public BaseCore {};

template <typename Result, typename Functor, typename Arg>
class Core : public ResultCore<Result> {
  using Base = ResultCore<Result>;

 public:
  Core(Functor&& functor) : ResultCore<Result>{}, _functor{std::move(functor)} {
  }
  Core(const Functor& functor) : _functor(functor) {
  }

  void SetPrev(container::intrusive::Ptr<ResultCore<Arg>> prev_state) {
    _prev = std::move(prev_state);
  }

  void ResetPrev() final {
    _prev = nullptr;
  }

 private:
  void Call() noexcept final {
    auto arg = _prev->GetResult();
    ResetPrev();

    if constexpr (std::is_void_v<Result>) {
      _functor(std::move(arg));
      Base::SetResult();
    } else {
      Base::SetResult(_functor(std::move(arg)));
    }
  }

 private:
  Functor _functor;
  container::intrusive::Ptr<ResultCore<Arg>> _prev;
};

template <typename Result, typename Functor>
class Core<Result, Functor, void> : public ResultCore<Result> {
  using Base = ResultCore<Result>;

 public:
  Core(Functor&& functor) : _functor{std::move(functor)} {
  }
  Core(const Functor& functor) : _functor(functor) {
  }

 private:
  void Call() noexcept final {
    if constexpr (std::is_void_v<Result>) {
      _functor();
      Base::SetResult();
    } else {
      Base::SetResult(_functor());
    }
  }

  Functor _functor;
};

template <typename Result>
class Core<Result, void, void> : public ResultCore<Result> {
  void Call() noexcept final {
    assert(false);
  }
};

template <typename T>
using PromiseCorePtr = container::intrusive::Ptr<Core<T, void, void>>;

template <typename Result>
using FutureCorePtr = container::intrusive::Ptr<ResultCore<Result>>;

}  // namespace yaclib::async

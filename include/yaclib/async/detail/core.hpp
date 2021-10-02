#pragma once

#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace yaclib::async::detail {

class BaseCore : public executor::ITask {
 protected:
  enum class State {
    Empty,
    HasResult,
    HasCallback,
    HasInlineCallback,
    HasWaitCallback,
    Stopped,
  };

 public:
  [[nodiscard]] executor::IExecutorPtr GetExecutor() const noexcept {
    return _executor;
  }
  [[nodiscard]] bool Ready() const noexcept {
    return _state.load(std::memory_order_acquire) == State::HasResult;
  }

  void SetExecutor(executor::IExecutorPtr executor) noexcept {
    _executor = std::move(executor);
  }

  void SetCallback(util::Ptr<ITask> callback) {
    _callback = std::move(callback);
    const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
    if (state == State::HasResult) {
      Execute();
    }
  }
  void Stop() noexcept {
    _state.store(State::Stopped, std::memory_order_release);
    assert(_callback == nullptr);
  }

  bool SetWaitCallback(util::Ptr<ITask> callback) noexcept {
    _callback = std::move(callback);
    const auto state = _state.exchange(State::HasWaitCallback, std::memory_order_acq_rel);
    const bool ready = state == State::HasResult;  // This is mean we have Result
    if (ready) {
      _state.store(State::HasResult, std::memory_order_release);
      _callback = nullptr;
    }
    return ready;
  }
  bool ResetAfterTimeout() noexcept {
    const auto state = _state.exchange(State::Empty, std::memory_order_acq_rel);
    const bool was_callback = state == State::HasWaitCallback;  // This is mean we don't have executed callback
    if (was_callback) {
      _callback = nullptr;
    }
    return was_callback;
  }

 protected:
  std::atomic<State> _state{State::Empty};
  util::Ptr<ITask> _caller;
  executor::IExecutorPtr _executor{executor::MakeInline()};
  util::Ptr<ITask> _callback;

  void Execute() {
    static_cast<BaseCore&>(*_callback)._caller = this;
    _executor->Execute(*_callback);
    // order is matter TODO(MBkkt) Why?
    _executor = nullptr;
    _callback = nullptr;
  }

  void Cancel() noexcept final {  // Opposite for Call with SetResult
    _caller = nullptr;
    // order is matter
    _executor = nullptr;
    _callback = nullptr;
  }
};

template <typename Value>
class ResultCore : public BaseCore {
 public:
  void SetResult(util::Result<Value>&& result) {
    _result = std::move(result);
    const auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    if (state == State::HasCallback) {
      BaseCore::Execute();
    } else if (state == State::HasInlineCallback) {
      _executor = executor::MakeInline();
      BaseCore::Execute();
    } else if (state == State::HasWaitCallback) {
      _callback = nullptr;
    } else if (state == State::Stopped) {
      BaseCore::Cancel();
    }
  }

  util::Result<Value>& Get() {
    return _result;
  }

 private:
  util::Result<Value> _result;
};

template <typename Ret, typename InvokeType, typename Arg>
class Core : public ResultCore<Ret> {
  using Base = ResultCore<Ret>;

 public:
  template <typename... T>
  explicit Core(T&&... t) : _functor{std::forward<T>(t)...} {
  }

 private:
  void Call() noexcept final {
    if (Base::_state.load(std::memory_order_acquire) == Base::State::Stopped) {
      Base::Cancel();
      return;
    }
    auto ret = [&] {
      auto* caller = static_cast<ResultCore<Arg>*>(Base::_caller.Get());
      if constexpr (std::is_void_v<Arg>) {
        if (!caller) {
          return util::Result<Arg>::Default();
        }
      }
      return std::move(caller->Get());
    }();
    Base::_caller = nullptr;
    Base::SetResult(_functor.Wrapper(std::move(ret)));
  }

  InvokeType _functor;
};

template <typename Result>
class Core<Result, void, void> : public ResultCore<Result> {
  void Call() noexcept final {
    assert(false);  // this class using only via promise
  }
};

class WaitCore : public executor::ITask {
 public:
  bool is_ready{false};
  std::mutex m;
  std::condition_variable cv;

 private:
  void Call() noexcept final {
  }
  void Cancel() noexcept final {
  }
};

class WaitCoreDeleter {
 public:
  template <typename Type>
  void Delete(void* p) {
    auto& self = *static_cast<WaitCore*>(p);
    std::lock_guard guard{self.m};
    self.is_ready = true;
    self.cv.notify_all();  // Notify under mutex, because cv located on stack memory of other thread
  }
};

template <typename Value>
using PromiseCorePtr = util::Ptr<Core<Value, void, void>>;

template <typename Value>
using FutureCorePtr = util::Ptr<ResultCore<Value>>;

}  // namespace yaclib::async::detail

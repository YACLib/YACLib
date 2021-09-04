#pragma once

#include <yaclib/container/intrusive_ptr.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/inline.hpp>
#include <yaclib/task.hpp>
#include <yaclib/util/result.hpp>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace yaclib::async::detail {

struct CallerCore : ITask {
  container::intrusive::Ptr<ITask> _caller;
};

class GetCore : public CallerCore {
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

class GetCoreDeleter {
 public:
  template <typename Type>
  void Delete(void* p) {
    auto& self = *static_cast<GetCore*>(p);
    self.m.lock();
    self.is_ready = self._caller != nullptr;
    self._caller = nullptr;
    self.cv.notify_all();  // Notify under mutex, because cv located on stack memory of other thread
    self.m.unlock();
  }
};

class BaseCore : public CallerCore {
 protected:
  enum class State {
    Empty,
    HasCallback,
    HasResult,
    Stopped,
  };

 public:
  bool IsReady() const noexcept {
    return _state.load(std::memory_order_acquire) != State::Empty;
  }

  void Stop() {
    _state.store(State::Stopped, std::memory_order_release);
    assert(_callback == nullptr);
  }

  void SetExecutor(executor::IExecutorPtr executor) noexcept {
    _executor = std::move(executor);
  }

  void SetCallback(container::intrusive::Ptr<ITask> callback) {
    _callback = std::move(callback);
    const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
    if (state == State::HasResult) {
      Execute();
    }
  }

  executor::IExecutorPtr GetExecutor() const noexcept {
    return _executor;
  }

 protected:
  std::atomic<State> _state{State::Empty};
  executor::IExecutorPtr _executor{executor::MakeInline()};
  container::intrusive::Ptr<ITask> _callback;

  void Cancel() noexcept final {
    _caller = nullptr;
    _callback = nullptr;
    _executor = nullptr;
  }

  void Execute() {
    assert(_caller == nullptr);
    static_cast<CallerCore&>(*_callback)._caller = this;
    _executor->Execute(*_callback);
    _callback = nullptr;
    _executor = nullptr;
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
    if (Base::_state.load(std::memory_order_acquire) == Base::State::Stopped) {
      Base::Cancel();
      return;
    }
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

template <typename Value>
using PromiseCorePtr = container::intrusive::Ptr<Core<Value, void, void>>;

template <typename Value>
using FutureCorePtr = container::intrusive::Ptr<ResultCore<Value>>;

}  // namespace yaclib::async::detail

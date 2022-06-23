#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/nope_counter.hpp>
#include <yaclib/util/ref.hpp>

namespace yaclib {

class OneShotEventAwaiter;

class OneShotEvent : public IRef {
 public:
  OneShotEvent() noexcept;

  void Set() noexcept;
  void Reset() noexcept;

  bool Ready() noexcept;

  OneShotEventAwaiter Await(IExecutor& executor);
  void Wait();

 private:
  static constexpr std::uintptr_t kEmpty = 0;
  static constexpr std::uintptr_t kAllDone = 1;
  friend OneShotEventAwaiter;

  bool TryAdd(Job* job) noexcept;
  yaclib_std::atomic_uintptr_t _head;
};

// TODO(mkornaukhov03)
// Non optimal, should store executor inside BaseCore
class OneShotEventAwaiter final : public detail::NopeCounter<Job> {
 public:
  OneShotEventAwaiter(IExecutor& executor, OneShotEvent& event) noexcept : _event{event}, _executor{executor} {
  }

  bool await_ready() const noexcept {
    return _event.Ready();
  }
  template <typename Promise>
  bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _core = &handle.promise();
    return _event.TryAdd(static_cast<Job*>(this));
  }
  void await_resume() const noexcept {
  }

  void Call() noexcept final {
    _executor.Submit(*_core);
  }
  void Cancel() noexcept final {
    _core->Cancel();
  }

 private:
  friend class OneShotEvent;
  OneShotEvent& _event;
  IExecutor& _executor;
  detail::BaseCore* _core = nullptr;
};

class OneShotEventWait : public Job, public detail::DefaultEvent {
 public:
  void Call() noexcept final {
    SetAll();
  }

  void Cancel() noexcept final {
    SetAll();
  }
};

void Wait(OneShotEvent& event);

}  // namespace yaclib

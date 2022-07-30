#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/nope_counter.hpp>
#include <yaclib/util/ref.hpp>

#include <tuple>

namespace yaclib {

class OneShotEventAwaiter;

class OneShotEvent : public IRef {
 public:
  OneShotEvent() noexcept;

  void Set() noexcept;
  void Reset() noexcept;

  bool Ready() noexcept;

  OneShotEventAwaiter Await(IExecutor& executor) noexcept;
  void Wait();

 private:
  static constexpr std::uint64_t kEmpty = 0;
  static constexpr std::uint64_t kAllDone = 1;
  friend OneShotEventAwaiter;

  bool TryAdd(Job* job) noexcept;
  yaclib_std::atomic_uint64_t _head;
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

  void Drop() noexcept final {  // LCOV_EXCL_LINE Never called
    _core->Drop();              // LCOV_EXCL_LINE
  }                             // LCOV_EXCL_LINE

 private:
  friend class OneShotEvent;
  OneShotEvent& _event;
  IExecutor& _executor;
  detail::BaseCore* _core = nullptr;
};

// TODO(mkornaukhov03) both Job and DefaultEvent inherited from IRef
class OneShotEventWait final : public detail::NopeCounter<Job, detail::DefaultEvent> {
 public:
  void Call() noexcept final {
    SetAll();
  }

  void Drop() noexcept final {
    SetAll();
  }
};

void Wait(OneShotEvent& event);

}  // namespace yaclib

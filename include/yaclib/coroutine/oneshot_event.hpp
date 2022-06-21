#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/ref.hpp>

namespace yaclib {

struct OneShotEventOperation;

struct OneShotEvent : public IRef {
  OneShotEvent() noexcept;
  void Set() noexcept;
  void Reset() noexcept;
  bool Ready() noexcept;

  OneShotEventOperation Wait(IExecutor&);

 private:
  static constexpr std::uintptr_t kEmpty = 0;
  static constexpr std::uintptr_t kAllDone = 1;
  friend class OneShotEventOperation;
  bool TryAdd(OneShotEventOperation*) noexcept;
  yaclib_std::atomic_uintptr_t _head;
};

// TODO(mkornaukhov03)
// Non optimal, should store executor inside BaseCore
struct OneShotEventOperation {
  OneShotEventOperation(IExecutor&, OneShotEvent&) noexcept;

  bool await_ready() const noexcept;
  template <typename Promise>
  bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _core = &handle.promise();
    return _event.TryAdd(this);
  }
  void await_resume() const noexcept;

 private:
  friend class OneShotEvent;
  OneShotEvent& _event;
  OneShotEventOperation* _next;
  detail::BaseCore* _core;
  IExecutor& _executor;
};
}  // namespace yaclib

#pragma once

#include <yaclib_std/detail/condition_variable.hpp>
#include <yaclib_std/detail/mutex.hpp>

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/util/ref.hpp>
namespace yaclib {

struct OneShotEventNode {
  virtual void Process() noexcept = 0;
  OneShotEventNode(OneShotEventNode*);
  OneShotEventNode* _next;
};

class OneShotEventOperation;

class OneShotEvent : public IRef {
 public:
  OneShotEvent() noexcept;
  void Set() noexcept;
  void Reset() noexcept;
  bool Ready() noexcept;

  OneShotEventOperation Await(IExecutor&);
  void Wait();

 private:
  static constexpr std::uintptr_t kEmpty = 0;
  static constexpr std::uintptr_t kAllDone = 1;
  friend class OneShotEventOperation;
  bool TryAdd(OneShotEventNode*) noexcept;
  yaclib_std::atomic_uintptr_t _head;
};

// TODO(mkornaukhov03)
// Non optimal, should store executor inside BaseCore
class OneShotEventOperation : public OneShotEventNode {
 public:
  OneShotEventOperation(IExecutor&, OneShotEvent&) noexcept;

  bool await_ready() const noexcept;
  template <typename Promise>
  bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _core = &handle.promise();
    return _event.TryAdd(this);
  }
  void await_resume() const noexcept;

  void Process() noexcept final;

 private:
  friend class OneShotEvent;
  OneShotEvent& _event;
  detail::BaseCore* _core;
  IExecutor& _executor;
};

class OneShotEventWait : public OneShotEventNode {
 public:
  OneShotEventWait(OneShotEvent&) noexcept;

  void Process() noexcept final;

 private:
  friend class OneShotEvent;
  OneShotEvent& _event;
  yaclib_std::mutex _mutex;
  yaclib_std::condition_variable _cv;
  bool _done = false;
};

YACLIB_INLINE void Wait(OneShotEvent& event);

}  // namespace yaclib

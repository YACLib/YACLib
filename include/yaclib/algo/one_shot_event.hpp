#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/nope_counter.hpp>
#include <yaclib/util/ref.hpp>

#include <tuple>

namespace yaclib {

class OneShotEventAwaiter;

class OneShotEvent : public IRef {
 public:
  OneShotEvent() noexcept;

  bool TryAdd(Job* job) noexcept;

  bool Ready() noexcept;

  void Wait();

#if YACLIB_CORO != 0
  OneShotEventAwaiter Await(IExecutor& executor = CurrentThreadPool()) noexcept;
#endif

  void Set() noexcept;

  void Reset() noexcept;

 private:
  static constexpr std::uint64_t kEmpty = 0;
  static constexpr std::uint64_t kAllDone = 1;

  yaclib_std::atomic_uint64_t _head;
};

#if YACLIB_CORO != 0

class OneShotEventAwaiter final : public detail::NopeCounter<Job> {
 public:
  OneShotEventAwaiter(IExecutor& executor, OneShotEvent& event) noexcept : _event{event}, _executor{executor} {
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return _event.Ready();
  }

  template <typename Promise>
  bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    _core = &handle.promise();
    return _event.TryAdd(this);
  }

  constexpr void await_resume() const noexcept {
  }

 private:
  void Call() noexcept final {
    _executor.Submit(*_core);
  }

  void Drop() noexcept final {  // LCOV_EXCL_LINE Never called
    _core->Drop();              // LCOV_EXCL_LINE
  }                             // LCOV_EXCL_LINE

  OneShotEvent& _event;
  IExecutor& _executor;
  detail::BaseCore* _core = nullptr;
};

inline auto operator co_await(OneShotEvent& event) noexcept {
  return event.Await();
}

#endif

}  // namespace yaclib

#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/nope_counter.hpp>
#include <yaclib/util/ref.hpp>
#if YACLIB_CORO != 0
#  include <yaclib/coroutine/coroutine.hpp>
#endif

namespace yaclib {

class OneShotEventAwaiter;

/**
 * TODO
 */
class OneShotEvent : public IRef {
 public:
  OneShotEvent() noexcept;

  /**
   * TODO
   */
  bool TryAdd(Job* job) noexcept;

  /**
   * TODO
   */
  bool Ready() noexcept;

  /**
   * TODO
   */
  void Wait() noexcept;

#if YACLIB_CORO != 0
  /**
   * TODO
   */
  OneShotEventAwaiter Await(IExecutor& executor = CurrentThreadPool()) noexcept;

  /**
   * TODO
   */
  OneShotEventAwaiter operator co_await() noexcept;
#endif

  /**
   * TODO
   */
  void Set() noexcept;

  /**
   * TODO
   */
  void Reset() noexcept;

 private:
  static constexpr std::uint64_t kEmpty = 0;
  static constexpr std::uint64_t kAllDone = 1;

  yaclib_std::atomic_uint64_t _head;
};

#if YACLIB_CORO != 0

class [[nodiscard]] OneShotEventAwaiter final : public detail::NopeCounter<Job> {
 public:
  YACLIB_INLINE explicit OneShotEventAwaiter(OneShotEvent& event, IExecutor& executor) noexcept
    : _event{event}, _executor{executor} {
  }

  YACLIB_INLINE bool await_ready() const noexcept {
    return _event.Ready();
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
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

YACLIB_INLINE OneShotEventAwaiter OneShotEvent::Await(IExecutor& executor) noexcept {
  return OneShotEventAwaiter{*this, executor};
}

YACLIB_INLINE OneShotEventAwaiter operator co_await(OneShotEvent& event) noexcept {
  return event.Await();
}

#endif

}  // namespace yaclib

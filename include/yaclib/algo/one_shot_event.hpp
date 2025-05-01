#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/mutex_event.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/ref.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/coro.hpp>
#endif

namespace yaclib {

class OneShotEventAwait;
class OneShotEventAwaitSticky;
class OneShotEventAwaitOn;

/**
 * This class useful to wait or co_await some event.
 *
 * In general it's MPSC pattern:
 * Multi threads can call TryAdd/Wait/Await*
 * Single thread can call Call or Set
 */
class OneShotEvent {
 public:
  /**
   * Add job to the MPSC event queue.
   * When Call or Set will be called also will be called job->Call
   *
   * But only if TryAdd return true.
   * It can return false if on Event already was called Set
   */
  bool TryAdd(Job& job) noexcept;

  /**
   * was or not Set
   */
  bool Ready() noexcept;

  /**
   * Wait Call or Set
   * immediately return if Event is Ready
   */
  void Wait() noexcept;

  /**
   * WaitFor Call or Set
   * immediately return if Event is Ready
   */
  template <typename Rep, typename Period>
  YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration) {
    return TimedWait(timeout_duration);
  }

  /**
   * WaitUntil Call or Set
   * immediately return if Event is Ready
   */
  template <typename Clock, typename Duration>
  YACLIB_INLINE bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return TimedWait(timeout_time);
  }

#if YACLIB_CORO != 0
 private:
  struct BaseAwaiter {
    explicit BaseAwaiter(OneShotEvent& event) noexcept : _event{event} {
    }

    constexpr void await_resume() const noexcept {
    }

   protected:
    OneShotEvent& _event;
  };

  struct ExtendedAwaiter : Job, BaseAwaiter {
    using BaseAwaiter::BaseAwaiter;

   protected:
    void Call() noexcept final {
      _core->_executor->Submit(*_core);
    }

    union {
      IExecutor* _executor;
      detail::BaseCore* _core;
    };
  };

  class [[nodiscard]] Awaiter final : public BaseAwaiter {
   public:
    using BaseAwaiter::BaseAwaiter;

    YACLIB_INLINE bool await_ready() const noexcept {
      return _event.Ready();
    }

    template <typename Promise>
    YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      return _event.TryAdd(handle.promise());
    }
  };

  class [[nodiscard]] StickyAwaiter final : public ExtendedAwaiter {
   public:
    using ExtendedAwaiter::ExtendedAwaiter;

    YACLIB_INLINE bool await_ready() const noexcept {
      return _event.Ready();
    }

    template <typename Promise>
    YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      _core = &handle.promise();
      return _event.TryAdd(*this);
    }
  };

  class [[nodiscard]] OnAwaiter final : public ExtendedAwaiter {
   public:
    explicit OnAwaiter(OneShotEvent& event, IExecutor& executor) noexcept : ExtendedAwaiter{event} {
      _executor = &executor;
    }

    constexpr bool await_ready() const noexcept {
      return false;
    }

    template <typename Promise>
    YACLIB_INLINE void await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
      auto& core = handle.promise();
      core._executor = _executor;
      _core = &core;
      if (!_event.TryAdd(*this)) {
        Call();
      }
    }
  };

 public:
  /**
   * co_await Call or Set
   * resume will be called inline
   *
   * immediately return if Event is Ready
   */
  YACLIB_INLINE Awaiter Await() noexcept {
    return Awaiter{*this};
  }

  /**
   * co_await Call or Set
   * resume will be called on this coroutine executor
   *
   * immediately return if Event is Ready
   */
  YACLIB_INLINE StickyAwaiter AwaitSticky() noexcept {
    return StickyAwaiter{*this};
  }

  /**
   * optimization for code like:
   * co_await event.Await();
   * co_await On(executor);
   */
  YACLIB_INLINE OnAwaiter AwaitOn(IExecutor& executor) noexcept {
    return OnAwaiter{*this, executor};
  }

  /**
   * just shortcut for co_await event.Await();
   *
   * TODO(MBkkt) move all shortcut to AwaitSticky
   */
  YACLIB_INLINE Awaiter operator co_await() noexcept {
    return Await();
  }
#endif

  /**
   * Get all jobs and Call them.
   */
  void Call() noexcept;

  /**
   * Prevent pushing new jobs and Call()
   */
  void Set() noexcept;

  /**
   * Reinitializes OneShotEvent, semantically the same as `*this = {};`
   *
   * If you don't explicitly call this method,
   * then after the first one, Wait will always return immediately.
   *
   * \note Not thread-safe!
   */
  void Reset() noexcept;

  /**
   * Waiter is public for advanced users.
   * Sometimes we don't want to recreate Waiter on every Wait call (it's created on stack).
   *
   * So we make Waiter public for such users, and they can write code like:
   *
   * Waiter _waiter;
   * OneShotEvent _event;
   * // code like OneShotEvent::Wait() but with our own _waiter
   * _event.Set();
   * // code like OneShotEvent::Wait() but with our own _waiter
   */
  struct Waiter : Job, detail::DefaultEvent {
    void Call() noexcept final {
      Set();
    }
  };

  /**
   * Public only because Waiter is public
   */
  struct TimedWaiter : Job, detail::MutexEvent {
    void Call() noexcept final {
      // TODO(MBkkt) Possible optimization: call Set() only if ref count != 1?
      Set();
      DecRef();
    }
  };

 private:
  template <typename Timeout>
  bool TimedWait(const Timeout& timeout) {
    auto waiter = MakeShared<TimedWaiter>(2);
    if (TryAdd(*waiter)) {
      auto token = waiter->Make();
      return waiter->Wait(token, timeout);
    }
    delete waiter.Release();
    return true;
  }

  static constexpr auto kEmpty = std::uintptr_t{0};
  static constexpr auto kAllDone = std::numeric_limits<std::uintptr_t>::max();

  yaclib_std::atomic_uintptr_t _head = kEmpty;
};

}  // namespace yaclib

#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/mutex_awaiter.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>
#include <yaclib/util/detail/intrusive_stack.hpp>
#include <yaclib/util/detail/spinlock.hpp>

#include <yaclib_std/atomic>
#include <yaclib_std/thread>

namespace yaclib {
namespace detail {

template <bool FIFO, bool WritersFIFO, bool ReadersFIFO>
struct SharedMutexImpl {
  [[nodiscard]] bool TryLockSharedAwait() noexcept {
    return _state.fetch_add(kReader, std::memory_order_acq_rel) / kWriter == 0;
  }

  [[nodiscard]] bool TryLockAwait() noexcept {
    // TODO(MBkkt) instead of TryLock we can do fetch_add, but needs to keep result for await_suspend
    return TryLock();
  }

  [[nodiscard]] bool AwaitLockShared(BaseCore& curr) noexcept {
    return Suspend(_readers_tail, curr);
  }

  [[nodiscard]] bool AwaitLock(BaseCore& curr) noexcept {
    auto s = _state.fetch_add(kWriter, std::memory_order_acq_rel);
    if (s / kWriter != 0) {
      return Suspend(_writers_tail, curr);
    }
    std::uint32_t r = s % kWriter;
    _writers_first = &curr;
    return r != 0 && _readers_wait.fetch_add(r, std::memory_order_acq_rel) != -r;
  }

  [[nodiscard]] bool TryLockShared() noexcept {
    auto s = _state.load(std::memory_order_relaxed);
    do {
      if (s / kWriter != 0) {
        return false;
      }
    } while (!_state.compare_exchange_weak(s, s + kReader, std::memory_order_acq_rel, std::memory_order_relaxed));
    return true;
  }

  [[nodiscard]] bool TryLock() noexcept {
    auto s = _state.load(std::memory_order_relaxed);
    return s == 0 && _state.compare_exchange_strong(s, kWriter, std::memory_order_acq_rel, std::memory_order_relaxed);
  }

  void UnlockHereShared() noexcept {
    if (auto s = _state.fetch_sub(kReader, std::memory_order_acq_rel); s >= kWriter) {
      // at least one writer waiting lock, so noone can acquire lock
      if (_readers_wait.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // last active reader will run writer
        _writers_first->_executor->Submit(*_writers_first);
      }
    }
  }

  void UnlockHere() noexcept {
    auto s = _state.fetch_sub(kWriter, std::memory_order_acq_rel) - kWriter;
    if (s == 0) {
      return;
    }
    if (s / kWriter == 0) {
      std::uint32_t r = s % kWriter;
      // We need to run exactly r readers from readers queue
      while (true) {
        while (_readers_head != nullptr) {
          Run(_readers_head);
          if (--r == 0) {
            return;
          }
        }
        Wait(_readers_tail);
        _readers_head = PopAll<ReadersFIFO>(_readers_tail);
      }
    }
    if (FIFO && _writers_head != nullptr) {
      // We need to run writer from current write batch
      return RunWriter();
    }
    // We want to run all readers from current readers queue
    auto r = RunReaders();
    _readers_head = PopAll<ReadersFIFO>(_readers_tail);
    r += RunReaders();
    if (FIFO || _writers_head == nullptr) {
      YACLIB_ASSERT(_writers_head == nullptr);
      // We need to wait at least single writer from writers queue
      Wait(_writers_tail);
      _writers_head = PopAll<WritersFIFO>(_writers_tail);
    }
    YACLIB_ASSERT(_writers_head != nullptr);
    _writers_first = _writers_head;
    _writers_head = static_cast<BaseCore*>(_writers_head->next);
    if (r == 0 || _readers_wait.fetch_add(r, std::memory_order_acq_rel) == -r) {
      _writers_first->_executor->Submit(*_writers_first);
    }
  }

 private:
  // 32 bit writers | 32 bit readers
  static constexpr auto kReader = std::uint64_t{1};
  static constexpr auto kWriter = kReader << std::uint64_t{32};

  void Run(Node* node) noexcept {
    YACLIB_ASSERT(node != nullptr);
    auto& core = static_cast<BaseCore&>(*node);
    core._executor->Submit(core);
  }

  static bool Suspend(yaclib_std::atomic<BaseCore*>& list, BaseCore& curr) noexcept {
    auto* head = list.load(std::memory_order_relaxed);
    do {
      curr.next = head;
    } while (list.compare_exchange_weak(head, &curr, std::memory_order_release, std::memory_order_relaxed));
    return true;
  }

  static void Wait(const yaclib_std::atomic<BaseCore*>& list) noexcept {
    while (list.load(std::memory_order_relaxed) == nullptr) {
      // yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }
  }

  template <bool Reverse>
  static BaseCore* PopAll(yaclib_std::atomic<BaseCore*>& list) noexcept {
    auto* tail = list.exchange(nullptr, std::memory_order_acquire);
    if constexpr (Reverse) {
      Node* node = tail;
      Node* prev = nullptr;
      while (node != nullptr) {
        auto* next = node->next;
        node->next = prev;
        prev = node;
        node = next;
      }
      return static_cast<BaseCore*>(prev);
    } else {
      return tail;
    }
  }

  static void Run(BaseCore*& head) noexcept {
    YACLIB_ASSERT(head != nullptr);
    auto* node = head;
    head = static_cast<BaseCore*>(head->next);
    auto& core = static_cast<BaseCore&>(*node);
    core._executor->Submit(core);
  }

  void RunWriter() noexcept {
    Run(_writers_head);
  }

  std::uint32_t RunReaders() noexcept {
    std::uint32_t r = 0;
    while (_readers_head != nullptr) {
      Run(_readers_head);
      ++r;
    }
    return r;
  }

  static constexpr auto kLockedNoWaiters = std::uintptr_t{0};
  static constexpr auto kNotLocked = std::numeric_limits<std::uintptr_t>::max();

  yaclib_std::atomic_uint64_t _state = 0;  // TODO(MBkkt) think about relax memory orders
  yaclib_std::atomic<BaseCore*> _writers_tail = nullptr;
  BaseCore* _writers_head = nullptr;
  yaclib_std::atomic<BaseCore*> _readers_tail = nullptr;
  BaseCore* _readers_head = nullptr;
  yaclib_std::atomic_uint32_t _readers_wait = 0;
  BaseCore* _writers_first = nullptr;
};

}  // namespace detail

/**
 * SharedMutex for coroutines
 *
 * \note It does not block execution thread, only coroutine
 * \note It is fair, with any options, so it doesn't allow recursive read locking
 *       and it's not possible to some coroutine wait other coroutines forever.
 * \note When we resume readers, we resume them all, no matter what options specified.
 *       Because otherwise will be more critical sections and I don't think it's good.
 *       If for some reason you need such behavior please use Semaphore.
 *
 * TODO(MBkkt) benchmark different options
 * \tparam FIFO -- if true readers and writers in "single" queue, on practice
 *         with false it will alternate writers and readers
 * \tparam ReadersFIFO -- readers resume order
 * \tparam WritersFIFO -- writers lock acquiring order
 * Configs:
 *  1 0 -- default, cares about honest order between critical sections that not intersects, but doesn't cares for other!
 *  0 0 -- cares only about throughput and liveness
 *  1 1 -- cares in first priority about order of critical sections
 *  0 1 -- opposite to default, but it's usefullness is doubtful
 */
template <bool FIFO = true, bool WritersFIFO = false, bool ReadersFIFO = false>
class SharedMutex final : protected detail::SharedMutexImpl<FIFO, WritersFIFO, ReadersFIFO> {
 public:
  using Base = detail::SharedMutexImpl<FIFO, WritersFIFO, ReadersFIFO>;

  using Base::Base;

  using Base::TryLock;

  using Base::TryLockShared;

  using Base::UnlockHere;

  using Base::UnlockHereShared;

  auto Lock() noexcept {
    return detail::LockAwaiter<Base, false>{*this};
  }

  auto LockShared() noexcept {
    return detail::LockAwaiter<Base, true>{*this};
  }

  auto TryGuard() noexcept {
    return UniqueGuard<SharedMutex>{*this, std::try_to_lock};
  }

  auto TryGuardShared() noexcept {
    return SharedGuard<SharedMutex>{*this, std::try_to_lock};
  }

  auto Guard() noexcept {
    return detail::GuardAwaiter<UniqueGuard, SharedMutex, false>{*this};
  }

  auto GuardShared() noexcept {
    return detail::GuardAwaiter<SharedGuard, SharedMutex, true>{*this};
  }

  // Helper for Awaiter implementation
  // TODO(MBkkt) get rid of it?
  template <typename To, typename From>
  static auto& Cast(From& from) noexcept {
    return static_cast<To&>(from);
  }
};

}  // namespace yaclib

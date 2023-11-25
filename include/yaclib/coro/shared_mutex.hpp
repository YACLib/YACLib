#pragma once

#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/mutex_awaiter.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/coro/guard.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>
#include <yaclib/util/detail/intrusive_stack.hpp>
#include <yaclib/util/detail/spinlock.hpp>

#include <yaclib_std/atomic>

namespace yaclib {
namespace detail {

template <bool FIFO, bool ReadersFIFO>
struct SharedMutexImpl {
  [[nodiscard]] bool TryLockSharedAwait() noexcept {
    return _state.fetch_add(kReader, std::memory_order_acq_rel) / kWriter == 0;
  }

  [[nodiscard]] bool TryLockAwait() noexcept {
    std::uint64_t s = 0;
    return _state.load(std::memory_order_relaxed) == s &&
           _state.compare_exchange_strong(s, s + kWriter, std::memory_order_acq_rel, std::memory_order_relaxed);
  }

  [[nodiscard]] bool AwaitLockShared(BaseCore& curr) noexcept {
    std::lock_guard lock{_lock};
    if (_readers_pass != 0) {
      --_readers_pass;
      return false;
    }
    YACLIB_ASSERT(_state.load(std::memory_order_relaxed) / kWriter != 0);
    _readers.PushBack(curr);
    ++_readers_size;
    return true;
  }

  [[nodiscard]] bool AwaitLock(BaseCore& curr) noexcept {
    curr.next = nullptr;
    std::lock_guard lock{_lock};
    auto s = _state.fetch_add(kWriter, std::memory_order_acq_rel);
    if (s / kWriter == 0) {
      std::uint32_t r = s % kWriter;
      _writers_first = &curr;
      return r != 0 && _readers_wait.fetch_add(r, std::memory_order_acq_rel) != -r;
    }
    _writers_tail->next = &curr;
    _writers_tail = &curr;
    if constexpr (FIFO) {
      _writers_prio += static_cast<std::uint32_t>(_readers.Empty());
    }
    return true;
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
    return TryLockAwait();
  }

  void UnlockHereShared() noexcept {
    if (auto s = _state.fetch_sub(kReader, std::memory_order_acq_rel); s >= kWriter) {
      // at least one writer waiting lock, so noone can acquire lock
      if (_readers_wait.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // last active reader will run writer
        Run(_writers_first);
      }
    }
  }

  void UnlockHere() noexcept {
    if (auto s = kWriter; !_state.compare_exchange_strong(s, 0, std::memory_order_acq_rel, std::memory_order_relaxed)) {
      SlowUnlock();
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

  void RunWriter() noexcept {
    if constexpr (FIFO) {
      YACLIB_ASSERT(_writers_prio != 0);
      --_writers_prio;
    }
    auto* node = _writers_head.next;
    _writers_head.next = node->next;
    if (_writers_head.next == nullptr) {
      _writers_tail = &_writers_head;
    }
    _lock.unlock();

    Run(node);
  }

  void PassReaders(std::uint64_t s) noexcept {
    YACLIB_ASSERT(s / kWriter == 1);
    std::uint32_t r = s % kWriter;
    YACLIB_ASSERT(r >= _readers_size);
    _readers_pass += r - _readers_size;
  }

  void RunReaders(std::uint64_t s) noexcept {
    if (std::uint32_t w = s / kWriter; w != 1) {
      _readers_wait.store(_readers_size, std::memory_order_relaxed);
      auto* node = _writers_head.next;
      _writers_head.next = node->next;
      if (_writers_head.next == nullptr) {
        _writers_tail = &_writers_head;
      }
      _writers_first = node;
      if constexpr (FIFO) {
        _writers_prio = w - 2;
      }
    } else {
      PassReaders(s);
    }
    auto readers = std::move(_readers);
    _readers_size = 0;
    _lock.unlock();

    do {
      Run(&readers.PopFront());
    } while (!readers.Empty());
  }

  void SlowUnlock() noexcept {
    _lock.lock();
    auto s = _state.fetch_sub(kWriter, std::memory_order_acq_rel);
    if constexpr (FIFO) {
      if (_writers_prio != 0) {
        YACLIB_ASSERT(s / kWriter > 1);
        return RunWriter();
      }
    }
    if (!_readers.Empty()) {
      return RunReaders(s);
    }
    if constexpr (!FIFO) {
      if (s / kWriter != 1) {
        return RunWriter();
      }
    }
    PassReaders(s);
    _lock.unlock();
  }

  yaclib_std::atomic_uint64_t _state = 0;  // TODO(MBkkt) think about relax memory orders
  std::conditional_t<ReadersFIFO, List, Stack> _readers;
  // TODO(MBkkt) add option for batched LIFO, see Mutex
  Node* _writers_first = nullptr;
  Node _writers_head;
  Node* _writers_tail = &_writers_head;
  std::uint32_t _writers_prio = 0;
  yaclib_std::atomic_uint32_t _readers_wait = 0;
  std::uint32_t _readers_size = 0;
  std::uint32_t _readers_pass = 0;
  Spinlock<std::uint32_t> _lock;
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
template <bool FIFO = true, bool ReadersFIFO = false>
class SharedMutex final : protected detail::SharedMutexImpl<FIFO, ReadersFIFO> {
 public:
  using Base = detail::SharedMutexImpl<FIFO, ReadersFIFO>;

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

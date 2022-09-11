#include <cstddef>
#include <limits>
#include <vector>
#include <yaclib_std/atomic>
#include <yaclib_std/mutex>
#include <yaclib/log.hpp>

namespace yaclib {

class Idle {
  static constexpr uint32_t kUnparkShift = 16U;
  static constexpr uint32_t kSearchMask = (1U << kUnparkShift) - 1U;
  static constexpr uint32_t kUnparkMask = ~kSearchMask;

 public:
  explicit Idle(std::uint16_t num_workers)
    : _state{static_cast<uint32_t>(num_workers) << kUnparkShift}, _num_workers{num_workers} {
    _sleepers.reserve(num_workers);
  }

  bool NotifyShouldWakeup() const noexcept {
    // State state = State(self.state.fetch_add(0, SeqCst));
    auto state = _state.load();
    // TODO(kononovk): maybe delete `state & kUnparkShift`
    return (state & kSearchMask) == 0 && ((state & kUnparkShift) >> kUnparkShift) < _num_workers;
  }

  // returns std::numeric_limits<std::uint16_t>::max() if no worker to notify
  std::uint16_t WorkerToNotify(bool force=false) noexcept {
    if (!force && !NotifyShouldWakeup()) {
      return std::numeric_limits<std::uint16_t>::max();
    }

    // Acquire the lock
    std::lock_guard lock{_sleepers_mutex};

    // Check again, now that the lock is acquired
    if (!force && !NotifyShouldWakeup()) {
      return std::numeric_limits<std::uint16_t>::max();
    }

    if (_sleepers.empty()) {
      YACLIB_ASSERT(force);
      return std::numeric_limits<std::uint16_t>::max();
    }

    // A worker should be woken up, atomically increment the number of
    // searching workers as well as the number of unparked workers.
    _state.fetch_add(1U | (1U << kUnparkShift));

    // Get the worker to unpark
    auto ret = _sleepers.back();
    _sleepers.pop_back();
    return ret;
  }

  /// Returns `true` if the worker needs to do a final check for submitted
  /// work.
  bool TransitionWorkerToParked(uint16_t worker, bool is_searching) {
    std::lock_guard lock{_sleepers_mutex};

    // Decrement the number of unparked threads
    auto dec = 1U << kUnparkShift;
    if (is_searching) {
      dec += 1;
    }
    auto prev = _state.fetch_sub(dec);
    bool dec_num_unparked = is_searching && (prev & kSearchMask) == 1;  // `true` if this is the final searching worker

    // Track the sleeping worker
    _sleepers.push_back(worker);

    return dec_num_unparked;
  }

  bool TransitionWorkerToSearching() {
    auto state = _state.load();
    if (2 * (state & kSearchMask) >= _num_workers) {
      return false;
    }
    // It is possible for this routine to allow more than 50% of the workers
    // to search. That is OK. Limiting searchers is only an optimization to
    // prevent too much contention.
    _state.fetch_add(1);
    return true;
  }

  /// A lightweight transition from searching -> running.
  ///
  /// Returns `true` if this is the final searching worker. The caller
  /// **must** notify a new worker.
  bool TransitionWorkerFromSearching() {
    auto state = _state.fetch_sub(1);
    return (state & kSearchMask) == 0;
  }

  /// Unpark a specific worker. This happens if tasks are submitted from
  /// within the worker's park routine.
  ///
  /// Returns `true` if the worker was parked before calling the method.
  bool UnparkWorkerById(uint16_t worker_id) {
    std::lock_guard lock{_sleepers_mutex};

    for (size_t index = 0; index < _sleepers.size(); ++index) {
      if (_sleepers[index] == worker_id) {
        _sleepers[index] = _sleepers.back();
        _sleepers.pop_back();

        // Update the state accordingly while the lock is held.
        _state.fetch_add(1U << kUnparkShift);
        return true;
      }
    }
    return false;
  }

  /// Returns `true` if `worker_id` is contained in the sleep set.
  bool IsParked(uint16_t worker_id) {
    std::lock_guard lock{_sleepers_mutex};
    for (uint16_t sleeping_worker : _sleepers) {
      if (sleeping_worker == worker_id) {
        return true;
      }
    }
    return false;
  }

  bool NotifyShouldWakeup() {
    auto state = _state.load();
    return (state & kSearchMask) == 0 && ((state & kUnparkMask) >> kUnparkShift) < _num_workers;
  }

 private:
  yaclib_std::atomic_uint32_t _state;
  std::uint16_t _num_workers;

  std::vector<std::uint16_t> _sleepers;  // sleeping workers
  mutable yaclib_std::mutex _sleepers_mutex;
};

}  // namespace yaclib

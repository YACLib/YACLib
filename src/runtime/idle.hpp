#include <yaclib/log.hpp>

#include <cstddef>
#include <limits>
#include <vector>
#include <yaclib_std/atomic>
#include <yaclib_std/mutex>

namespace yaclib {

class Idle final {
  static constexpr std::uint32_t kUnparkShift = 16U;
  static constexpr std::uint32_t kSearchMask = (1U << kUnparkShift) - 1U;
  static constexpr std::uint32_t kUnparkMask = ~kSearchMask;

 public:
  explicit Idle(std::uint16_t num_workers)
    : _state{static_cast<std::uint32_t>(num_workers) << kUnparkShift}, _num_workers{num_workers} {
    _sleepers.reserve(num_workers);
  }

  [[nodiscard]] bool TransitionWorkerToSearching() noexcept {
    auto const state = _state.load();
    // It is possible for this routine to allow more than 50% of the workers
    // to search. That is OK.
    // Limiting searchers is only an optimization to prevent too much contention.
    if (2 * (state & kSearchMask) >= _num_workers) {
      return false;
    }
    _state.fetch_add(1);
    return true;
  }

  // Returns `true` if the worker needs to do a final check for submitted work.
  [[nodiscard]] bool TransitionWorkerToParked(std::uint16_t worker, bool is_searching) noexcept {
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

  // Returns `true` if `worker_id` is contained in the sleep set.
  [[nodiscard]] bool IsParked(std::uint16_t worker) const noexcept {
    std::lock_guard lock{_sleepers_mutex};
    for (auto const sleeper : _sleepers) {
      if (sleeper == worker) {
        return true;
      }
    }
    return false;
  }

  // A lightweight transition from searching -> running.
  //
  // Returns `true` if this is the final searching worker.
  // The caller **must** notify a new worker.
  [[nodiscard]] bool TransitionWorkerFromSearching() noexcept {
    auto const state = _state.fetch_sub(1);
    return (state & kSearchMask) == 0;
  }

  // returns std::numeric_limits<std::uint16_t>::max() if no worker to notify
  [[nodiscard]] std::uint16_t WorkerToNotify() noexcept {
    if (!NotifyShouldWakeup()) {
      return std::numeric_limits<std::uint16_t>::max();
    }

    // Acquire the lock
    std::lock_guard lock{_sleepers_mutex};

    // Check again, now that the lock is acquired
    if (!NotifyShouldWakeup()) {
      return std::numeric_limits<std::uint16_t>::max();
    }

    // A worker should be woken up, atomically increment the number of
    // searching workers as well as the number of unparked workers.
    _state.fetch_add(1U | (1U << kUnparkShift));

    // Get the worker to unpark
    auto const sleeper = _sleepers.back();
    _sleepers.pop_back();
    return sleeper;
  }

  /// Unpark a specific worker. This happens if tasks are submitted from
  /// within the worker's park routine.
  ///
  /// Returns `true` if the worker was parked before calling the method.
  [[nodiscard]] bool UnparkWorkerById(std::uint16_t worker) noexcept {
    std::lock_guard lock{_sleepers_mutex};
    for (auto& sleeper : _sleepers) {
      if (sleeper == worker) {
        sleeper = _sleepers.back();
        _sleepers.pop_back();

        // Update the state accordingly while the lock is held.
        _state.fetch_add(1U << kUnparkShift);
        return true;
      }
    }
    return false;
  }

 private:
  [[nodiscard]] bool NotifyShouldWakeup() const noexcept {
    auto const state = _state.load();  // fetch_add(0, seq_cst)?
    return (state & kSearchMask) == 0 && ((state & kUnparkMask) >> kUnparkShift) < _num_workers;
  }

  yaclib_std::atomic_uint32_t _state;
  std::uint32_t _num_workers;

  std::vector<std::uint16_t> _sleepers;  // sleeping workers
  mutable yaclib_std::mutex _sleepers_mutex;
};

}  // namespace yaclib

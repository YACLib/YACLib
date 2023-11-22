#pragma once

#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename T>
class Spinlock {
 public:
  void lock() noexcept {
    while (_state.exchange(1, std::memory_order_acquire) != 0) {
      do {
        // TODO(MBkkt) pause, yield, sleep
      } while (_state.load(std::memory_order_relaxed) != 0);
    }
  }

  void unlock() noexcept {
    _state.store(0, std::memory_order_release);
  }

 private:
  yaclib_std::atomic<T> _state = 0;
};

}  // namespace yaclib::detail

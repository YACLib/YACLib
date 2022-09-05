#pragma once

#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <tuple>
#include <utility>
#include <yaclib_std/atomic>

namespace yaclib::detail {

/**
 * Single producer / multiple consumers bounded queue
 * for local tasks
 *
 *  @note TryPush, TryPop, Grab concurrently work only with Grab
 */
template <size_t Capacity>
class alignas(kCacheLineSize) WorkStealingQueue {
  static_assert(Capacity < std::numeric_limits<uint16_t>::max());

 public:
  /**
   * Single producer
   */
  bool TryPush(Job* item) noexcept {
    const auto head = _head.load(std::memory_order_acquire);
    const auto [steal, real] = Unpack(head);
    const auto tail = _tail.load(std::memory_order_relaxed);
    if (tail - steal >= Capacity) {
      return false;
    }
    const auto idx = tail % Capacity;
    _buffer[idx] = item;
    _tail.store(tail + 1, std::memory_order_release);
    return true;
  }

  /// Multiple consumers

  /**
   * Returns nullptr if queue is empty
   */
  Job* TryPop() noexcept {
    auto head = _head.load(std::memory_order_acquire);
    std::uint16_t steal{};
    std::uint16_t real{};
    std::uint32_t next{};
    do {
      std::tie(steal, real) = Unpack(head);
      auto tail = _tail.load(std::memory_order_relaxed);
      if (tail == real) {
        return nullptr;
      }
      if (steal == real) {
        next = Pack(real + 1, real + 1);
      } else {
        YACLIB_ASSERT(steal != real + 1);
        next = Pack(steal, real + 1);
      }
    } while (!_head.compare_exchange_weak(head, next, std::memory_order_acq_rel, std::memory_order_acquire));
    const auto idx = real % Capacity;
    return _buffer[idx];
  }

  /**
   * For stealing and for offloading to global queue
   * Returns number of tasks in `out_buffer`
   */
  size_t Grab(std::span<Job*> out_buffer) noexcept {
    auto head = _head.load(std::memory_order_acquire);
    std::uint16_t n{};
    std::uint16_t steal{};
    std::uint16_t real{};
    std::uint32_t next{};
    do {
      std::tie(steal, real) = Unpack(head);
      if (steal != real) {
        return 0;
      }
      auto tail = _tail.load(std::memory_order_acquire);
      n = std::min(static_cast<uint16_t>(tail - steal), static_cast<uint16_t>(out_buffer.size()));
      if (n == 0) {
        return 0;
      }
      const auto real_to = steal + n;
      YACLIB_ASSERT(steal != real_to);
      next = Pack(steal, real_to);
    } while (!_head.compare_exchange_weak(head, next, std::memory_order_acq_rel, std::memory_order_acquire));
    for (uint16_t i = 0; i != n; ++i) {
      auto idx = (steal + i) % Capacity;
      out_buffer[i] = _buffer[idx];
    }
    head = next;
    do {
      std::tie(steal, real) = Unpack(head);
      next = Pack(real, real);
    } while (!_head.compare_exchange_weak(head, next, std::memory_order_acq_rel, std::memory_order_acquire));
    return n;
  }

  /**
   * Clear WorkStealingQueue
   */
  void Clear() noexcept {
    // TODO(kononovk): make more optimal
    while (auto* job = TryPop()) {
      job->Drop();
    }
  }

  bool Empty() const noexcept {
    std::uint16_t real{};
    std::tie(std::ignore, real) = Unpack(_head.load(std::memory_order_acquire));
    return _tail.load(std::memory_order_acquire) == real;
  }

 private:
  static constexpr uint8_t kShift = 16;
  static uint32_t Pack(uint16_t steal, uint16_t real) noexcept {
    return (static_cast<uint32_t>(steal) << kShift) | static_cast<uint32_t>(real);
  }

  static std::pair<uint16_t, uint16_t> Unpack(uint32_t head) noexcept {
    return {static_cast<uint16_t>(head >> kShift), static_cast<uint16_t>(head & std::numeric_limits<uint16_t>::max())};
  }

  yaclib_std::atomic<uint32_t> _head{0};
  std::array<Job*, Capacity> _buffer{};
  yaclib_std::atomic<uint16_t> _tail{0};  // TODO(kononovk, MBkkt): atomic_ref
};

}  // namespace yaclib::detail

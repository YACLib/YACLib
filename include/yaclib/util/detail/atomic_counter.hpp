#pragma once

#include <yaclib/config.hpp>
#include <yaclib/util/detail/default_deleter.hpp>

#include <cstddef>
#include <utility>
#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename CounterBase, typename Deleter = DefaultDeleter>
struct AtomicCounter : CounterBase {
  template <typename... Args>
  AtomicCounter(std::size_t n, Args&&... args) noexcept(std::is_nothrow_constructible_v<CounterBase, Args&&...>)
    : CounterBase{std::forward<Args>(args)...}, count{n} {
  }

  YACLIB_INLINE void Add(std::size_t delta) noexcept {
    count.fetch_add(delta, std::memory_order_relaxed);
  }

  YACLIB_INLINE void Sub(std::size_t delta) noexcept {
    if (SubEqual(delta)) {
      Deleter::Delete(*this);
    }
  }

  // Dangerous! Use only to sync with release with acquire or with relaxed if synchronization is not needed
  [[nodiscard]] YACLIB_INLINE std::size_t Get(std::memory_order order = std::memory_order_relaxed) const noexcept {
    return count.load(order);
  }

  [[nodiscard]] YACLIB_INLINE bool SubEqual(std::size_t n) noexcept {
#ifdef YACLIB_TSAN
    return count.fetch_sub(n, std::memory_order_acq_rel) == n;
#else
    // Thread Sanitizer have false positive error with std::atomic_thread_fence
    // https://www.boost.org/doc/libs/1_76_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_reference_counters
    if (count.fetch_sub(n, std::memory_order_release) == n) {
      yaclib_std::atomic_thread_fence(std::memory_order_acquire);
      return true;
    }
    return false;
#endif
  }

  yaclib_std::atomic_size_t count;
};

}  // namespace yaclib::detail

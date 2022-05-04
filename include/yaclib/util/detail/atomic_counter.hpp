#pragma once

#include <yaclib/config.hpp>

#include <cstddef>
#include <utility>
#include <yaclib_std/atomic>

namespace yaclib::detail {

struct [[maybe_unused]] DefaultDeleter {
  template <typename Type>
  static void Delete(Type& self) {
    delete &self;
  }
};

template <typename CounterBase, typename Deleter = DefaultDeleter>
struct AtomicCounter : CounterBase {
  using DeleterType = Deleter;

  template <typename... Args>
  AtomicCounter(std::size_t n, Args&&... args) : CounterBase{std::forward<Args>(args)...}, count{n} {
  }

  void IncRef() noexcept final {
    count.fetch_add(1, std::memory_order_relaxed);
  }

  void DecRef() noexcept final {
    if (SubEqual(1)) {
      Deleter::Delete(*this);
    }
  }

  [[nodiscard]] std::size_t GetRef() const noexcept {
    // Dangerous! Use only to sync with release or if synchronization is not needed
    return count.load(std::memory_order_acquire);
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

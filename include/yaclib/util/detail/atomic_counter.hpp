#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/atomic.hpp>

#include <cstddef>
#include <utility>

namespace yaclib::detail {

struct [[maybe_unused]] DefaultDeleter {
  template <typename Type>
  static void Delete(Type* self) {
    delete self;
  }
};

template <typename CounterBase, typename Deleter = detail::DefaultDeleter>
struct AtomicCounter : CounterBase {
  template <typename... Args>
  AtomicCounter(std::size_t n, Args&&... args) : CounterBase{std::forward<Args>(args)...}, count{n} {
  }

  void IncRef() noexcept final {
    count.fetch_add(1, std::memory_order_relaxed);
  }

  void DecRef() noexcept final {
#ifdef YACLIB_TSAN
    if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
#else
    // Thread Sanitizer have false positive error with std::atomic_thread_fence
    // https://www.boost.org/doc/libs/1_76_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_reference_counters
    if (count.fetch_sub(1, std::memory_order_release) == 1) {
      yaclib_std::atomic_thread_fence(std::memory_order_acquire);
#endif
      Deleter::Delete(this);
    }
  }

  std::size_t GetRef() const noexcept {  // Dangerous! Use only to sync with release or if synchronization is not needed
    return count.load(std::memory_order_acquire);
  }

  bool SubEqual(std::size_t n) noexcept {
    return count.fetch_sub(n, std::memory_order_acq_rel) == n;
  }

  yaclib_std::atomic_size_t count;
};

}  // namespace yaclib::detail

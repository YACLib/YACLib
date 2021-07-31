#pragma once

#include <yaclib/config.hpp>

#include <atomic>

namespace yaclib::container {
namespace detail {

struct DefaultDeleter {
  template <typename Type>
  void Delete(void* p) {
    delete static_cast<Type>(p);
  }
};

}  // namespace detail

template <typename Base, typename Deleter = detail::DefaultDeleter>
class Counter final : public Base, public Deleter {
 public:
  using Base::Base;

  void IncRef() noexcept final {
    _impl.fetch_add(1, std::memory_order_relaxed);
  }

  void DecRef() noexcept final {
#ifdef YACLIB_TSAN
    if (_impl.fetch_sub(1, std::memory_order_acq_rel) == 1) {
#else
    // Thread Sanitizer have false positive error with std::atomic_thread_fence
    // https://www.boost.org/doc/libs/1_76_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_reference_counters
    if (_impl.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
#endif
      Deleter::template Delete<decltype(this)>(this);
    }
  }

  size_t GetRef() const noexcept {  // Only for tests
    return _impl.load(std::memory_order_relaxed);
  }

 private:
  std::atomic_size_t _impl{0};
};

template <typename Base>
class NothingCounter final : public Base {
 public:
  using Base::Base;

  void IncRef() noexcept final {
  }

  void DecRef() noexcept final {
  }
};

}  // namespace yaclib::container

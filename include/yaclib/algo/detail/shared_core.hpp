#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/algo/detail/core_util.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <limits>

namespace yaclib::detail {

class SharedBaseCore : public InlineCore {
 public:
  static constexpr auto kSet = std::numeric_limits<std::uintptr_t>::max();

  [[nodiscard]] bool SetCallback(InlineCore& callback) noexcept {
    auto* next = _head.load(std::memory_order_acquire);
    do {
      if (reinterpret_cast<std::uintptr_t>(next) == kSet) {
        return false;
      }
      callback.next = next;
    } while (!_head.compare_exchange_weak(next, &callback, std::memory_order_release, std::memory_order_acquire));
    return true;
  }

  void FulfillQueue(InlineCore* head) noexcept {
    while (head) {
      auto next = head->next;
      Loop(this, head);
      head = static_cast<InlineCore*>(next);
    }
    DecRef();
    DecRef();
  }

  bool IsSet() const noexcept {
    return reinterpret_cast<std::uintptr_t>(_head.load(std::memory_order_relaxed)) == kSet;
  }

 protected:
  yaclib_std::atomic<InlineCore*> _head = nullptr;
};

template <typename V, typename E>
class SharedCore : public SharedBaseCore {
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    InlineCore* head = [&]() {
      if (caller.GetRef() != 1) {
        return Store(ResultFromCore<V, E, true>(caller));
      } else {
        auto head = Store(ResultFromCore<V, E, false>(caller));
        caller.DecRef();
        return head;
      }
    }();
    FulfillQueue(head);
    return Noop<SymmetricTransfer>();
  }

 public:
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return Impl<true>(caller);
  }
#endif

 public:
  SharedCore() noexcept {
  }

  template <typename... Args>
  InlineCore* Store(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>) {
    new (&_result) Result<V, E>{std::forward<Args>(args)...};

    auto head = _head.exchange(reinterpret_cast<InlineCore*>(kSet), std::memory_order_acq_rel);
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(head) != kSet);

    return head;
  }

  ~SharedCore() noexcept override {
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(_head.load(std::memory_order_relaxed)) == kSet);
    _result.~Result<V, E>();
  }

  const Result<V, E>& Get() const noexcept {
    return _result;
  }

 private:
  union {
    Result<V, E> _result;
  };
};

template <typename V, typename E>
using SharedCorePtr = IntrusivePtr<SharedCore<V, E>>;

}  // namespace yaclib::detail

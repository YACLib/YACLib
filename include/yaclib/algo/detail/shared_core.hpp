#pragma once

#include "yaclib/fwd.hpp"

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/util/ref.hpp>

#include <atomic>

namespace yaclib {
namespace detail {

template <typename V, typename E>
class SharedCore : public IRef {
  using ResultCoreType = ResultCore<V, E>;
  using ResultCorePtrType = detail::ResultCorePtr<V, E>;

 public:
  SharedCore() noexcept : _stub{} {};

  void Attach(Promise<V, E>&& p) {
    ResultCoreType* core = p.GetCore().Get();
    ResultCoreType* next = _head.load(std::memory_order_acquire);
    do {
      if (reinterpret_cast<std::uintptr_t>(next) == kSet) {
        std::move(p).Set(_result);
        break;
      }
      core->next = next;
    } while (!_head.compare_exchange_weak(next, core, std::memory_order_release, std::memory_order_acquire));
    p.GetCore().Release();
  }

  template <typename... Args>
  void Set(Args&&... args) {
    new (&_result) Result<V, E>{std::forward<Args>(args)...};

    auto head = _head.exchange(reinterpret_cast<ResultCoreType*>(kSet), std::memory_order_acq_rel);
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(head) != kSet);

    while (head) {
      auto next = head->next;
      Promise<V, E>{detail::ResultCorePtr<V, E>{NoRefTag{}, head}}.Set(_result);
      head = static_cast<ResultCoreType*>(next);
    }
  }

  ~SharedCore() {
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(_head.load(std::memory_order_relaxed)) == kSet);
    _result.~Result<V, E>();
  }

 private:
  static constexpr std::uintptr_t kSet = std::numeric_limits<std::uintptr_t>::max();

  yaclib_std::atomic<ResultCoreType*> _head = nullptr;
  union {
    Unit _stub;
    Result<V, E> _result;
  };
};

template <typename V, typename E>
using SharedCorePtr = IntrusivePtr<SharedCore<V, E>>;

}  // namespace detail
}  // namespace yaclib

#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/detail/handle_wrapper.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

#include <iostream>  // for debug

namespace yaclib {

namespace detail {
// TODO(mkornaukhov03) memory models
struct LFStack : public IRef {
  yaclib_std::atomic<std::uintptr_t> head{kEmpty};
  bool AddWaiter(BaseCore* core_ptr) {  // TODO noexcept?
    std::uintptr_t old_head = head.load(std::memory_order_seq_cst);
    std::uintptr_t core = reinterpret_cast<std::uintptr_t>(core_ptr);
    while (old_head != kAllDone) {
      core_ptr->next = reinterpret_cast<BaseCore*>(old_head);
      if (head.compare_exchange_weak(old_head, core)) {
        return true;
      }
    }
    return false;
  }
  constexpr static std::uintptr_t kEmpty = 0;
  constexpr static std::uintptr_t kAllDone = 1;
};
struct AwaitersResumer {
  static void Delete(LFStack& awaiters) {
    // std::cout << "Resumer Delete" << std::endl;
    std::uintptr_t old_head = awaiters.head.exchange(LFStack::kAllDone, std::memory_order_seq_cst);  // TODO noexcept?
    int i = 0;
    while (old_head != LFStack::kEmpty) {
      BaseCore* core = reinterpret_cast<BaseCore*>(old_head);
      yaclib_std::coroutine_handle<> handle = core->GetHandle();
      old_head = reinterpret_cast<std::uintptr_t>(static_cast<BaseCore*>(core->next));
      handle.resume();  // TODO(mkornaukhov03, MBkkt) run in custom executor
    }
  }
};

}  // namespace detail

class AwaitGroup {
 public:
  AwaitGroup() = default;
  void Add(std::size_t count = 1) noexcept {
    _await_core.IncRef(count);
  }
  void Done(std::size_t count = 1) noexcept {
    _await_core.DecRef(count);
  }
  template <bool NeedAdd = true, typename... V, typename... E>
  YACLIB_INLINE void Add(FutureBase<V, E>&... futures) {
    AddCore<NeedAdd>(static_cast<detail::BaseCore&>(*futures.GetCore())...);
  }
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> Add(It begin, It end) {
    AddIterator<NeedAdd>(begin, end - begin);
  }

  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE void Add(It begin, std::size_t count) {
    AddIterator<NeedAdd>(begin, count);
  }

  // Awaitable traits

  YACLIB_INLINE bool await_ready() const noexcept {
    return _await_core.head.load(std::memory_order_seq_cst) == detail::LFStack::kAllDone;
  }

  template <typename Promise>
  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<Promise> handle) noexcept {
    detail::BaseCore& core = handle.promise();
    return _await_core.AddWaiter(&core);
  }

  YACLIB_INLINE void await_resume() const noexcept {
  }

 private:
  template <bool NeedAdd, typename... Cores>
  void AddCore(Cores&... cores) {
    static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
    static_assert((... && std::is_same_v<detail::BaseCore, Cores>),
                  "Futures must be Future in WaitGroup::Add function");
    auto range = [&](auto&& func) {
      return (... + static_cast<std::size_t>(func(cores)));
    };
    AddRange<NeedAdd>(range, sizeof...(cores));
  }

  template <bool NeedAdd, typename Iterator>
  void AddIterator(Iterator it, std::size_t count) {
    static_assert(is_future_base_v<typename std::iterator_traits<Iterator>::value_type>,
                  "WaitGroup::Add function Iterator must be point to some Future");
    if (count == 0) {
      return;
    }
    auto range = [&](auto&& func) {
      std::size_t wait_count = 0;
      for (std::size_t i = 0; i != count; ++i) {
        wait_count += static_cast<std::size_t>(func(*it->GetCore()));
        ++it;
      }
      return wait_count;
    };
    AddRange<NeedAdd>(range, count);
  }

  template <bool NeedAdd, typename Range>
  void AddRange(const Range& range, std::size_t count) {
    if constexpr (NeedAdd) {
      Add(count);
    }
    const auto wait_count = range([&](detail::BaseCore& core) {
      return core.Empty() && core.SetWait(_await_core);
    });
    Done(count - wait_count);  // TODO(MBkkt) Maybe add if about wait_count == count?
  }

  detail::AtomicCounter<detail::LFStack, detail::AwaitersResumer> _await_core{0};
};

}  // namespace yaclib

#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/detail/handle_wrapper.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {
template <std::size_t InitCount = 1>
class AwaitGroup {
 public:
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
    return _await_core.GetRef() == 1;
  }

  YACLIB_INLINE bool await_suspend(yaclib_std::coroutine_handle<> handle) noexcept {
    _await_core.handle = std::move(handle);
    return !_await_core.SubEqual(1);
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

  detail::AtomicCounter<detail::Handle, detail::HandleDeleter> _await_core{InitCount};
};

}  // namespace yaclib

/*

coro {
  AwaitGroup ag;
  for (auto && f : fs) {
    if (pred) ag.add(f);
  }

  ...
  co_await ag()
}

 */

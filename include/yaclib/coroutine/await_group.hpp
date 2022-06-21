#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/detail/awaiter_deleters.hpp>
#include <yaclib/coroutine/oneshot_event.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

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

  auto Wait(IExecutor& exec = CurrentThreadPool()) {
    return _await_core.Wait(exec);
  }

  auto operator co_await() {
    YACLIB_INFO(true, "Better use AwaitGroup::Wait(executor)");
    return Wait();
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

  struct OneShotEventShoter {
    static void Delete(OneShotEvent& event) {
      event.Set();
    }
  };

  detail::AtomicCounter<OneShotEvent, OneShotEventShoter> _await_core{0};
};

}  // namespace yaclib

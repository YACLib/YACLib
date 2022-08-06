#pragma once

namespace yaclib {

template <bool NeedAdd, typename... Cores>
void WaitGroup::AddCore(Cores&... cores) {
  static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<detail::BaseCore, Cores>), "Futures must be Future in WaitGroup::Add function");
  auto range = [&](auto&& func) noexcept {
    return (... + static_cast<std::size_t>(func(cores)));
  };
  AddRange<NeedAdd>(range, sizeof...(cores));
}

template <bool NeedAdd, typename Iterator>
void WaitGroup::Add(Iterator it, std::size_t count) {
  static_assert(is_future_base_v<typename std::iterator_traits<Iterator>::value_type>,
                "WaitGroup::Add function Iterator must be point to some Future");
  if (count == 0) {
    return;
  }
  auto range = [&](auto&& func) noexcept {
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
void WaitGroup::AddRange(const Range& range, std::size_t count) {
  if constexpr (NeedAdd) {
    Add(count);
  }
  const auto wait_count = range([&](detail::BaseCore& core) noexcept {
    return core.SetWait(_await_core, detail::InlineCore::kWaitNope);
  });
  Done(count - wait_count);  // TODO(MBkkt) Maybe add if about wait_count == count?
}

template <bool NeedAdd, typename... V, typename... E>
YACLIB_INLINE void WaitGroup::Add(FutureBase<V, E>&... futures) {
  AddCore<NeedAdd>(static_cast<detail::BaseCore&>(*futures.GetCore())...);
}

template <bool NeedAdd, typename It>
YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> WaitGroup::Add(It begin, It end) {
  Add<NeedAdd>(begin, end - begin);
}

YACLIB_INLINE void Wait(WaitGroup& wg) {
  wg.Wait();
}

}  // namespace yaclib

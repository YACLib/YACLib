#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace yaclib {

// TODO(MBkkt) Reduce duplication between wait_impl, wait_group_impl, await_impl, await_group_impl

template <std::size_t InitCount, typename Event>
void WaitGroup<InitCount, Event>::Add(std::size_t count) {
  _event.count.fetch_add(count, std::memory_order_relaxed);
}

template <std::size_t InitCount, typename Event>
void WaitGroup<InitCount, Event>::Done(std::size_t count) {
  if (_event.SubEqual(count)) {
    EventCore::DeleterType::Delete(_event);
  }
}

template <std::size_t InitCount, typename Event>
template <bool NeedDone>
void WaitGroup<InitCount, Event>::Wait() {
  if constexpr (NeedDone) {
    Done();
  }
  auto token = _event.Make();
  _event.Wait(token);
}

template <std::size_t InitCount, typename Event>
void WaitGroup<InitCount, Event>::Reset() {
  _event.Reset();
  _event.count.store(InitCount, std::memory_order_relaxed);
}

template <std::size_t InitCount, typename Event>
template <bool NeedAdd, typename... Cores>
void WaitGroup<InitCount, Event>::AddCore(Cores&... cores) {
  static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<detail::BaseCore, Cores>), "Futures must be Future in WaitGroup::Add function");
  auto range = [&](auto&& func) {
    return (... + static_cast<std::size_t>(func(cores)));
  };
  AddRange<NeedAdd>(range, sizeof...(cores));
}

template <std::size_t InitCount, typename Event>
template <bool NeedAdd, typename Iterator>
void WaitGroup<InitCount, Event>::AddIterator(Iterator it, std::size_t count) {
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

template <std::size_t InitCount, typename Event>
template <bool NeedAdd, typename Range>
void WaitGroup<InitCount, Event>::AddRange(const Range& range, std::size_t count) {
  if constexpr (NeedAdd) {
    Add(count);
  }
  const auto wait_count = range([&](detail::BaseCore& core) {
    return core.Empty() && core.SetWait(_event);
  });
  Done(count - wait_count);  // TODO(MBkkt) Maybe add if about wait_count == count?
}

}  // namespace yaclib

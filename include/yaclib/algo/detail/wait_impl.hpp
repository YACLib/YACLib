#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/set_one_deleter.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace yaclib::detail {

struct NoTimeoutTag final {};

template <typename Event, typename Timeout, typename Range>
bool WaitRange(const Timeout& timeout, const Range& range, std::size_t count) noexcept {
  AtomicCounter<Event, SetOneDeleter> event{count + 1};
  // event ref counter = n + 1, it is optimization: we don't want to notify when return true immediately
  auto const wait_count = range([&](BaseCore& core) noexcept {
    return core.SetWait(event, InlineCore::kWaitNope);
  });
  if (wait_count == 0 || event.SubEqual(count - wait_count + 1)) {
    return true;
  }
  auto token = event.Make();
  std::size_t reset_count = 0;
  if constexpr (!std::is_same_v<Timeout, NoTimeoutTag>) {
    // If you have problem with TSAN here, check this link: https://github.com/google/sanitizers/issues/1259
    // TLDR: new pthread function is not supported by thread sanitizer yet.
    if (event.Wait(token, timeout)) {
      return true;
    }
    reset_count = range([](BaseCore& core) noexcept {
      return core.ResetWait();
    });
    if (reset_count != 0 && (reset_count == wait_count || event.SubEqual(reset_count))) {
      return false;
    }
    // We know we have `wait_count - reset_count` Results, but we must wait until event was not used by cores
  }
  event.Wait(token);
  return reset_count == 0;
}

template <typename Event, typename Timeout, typename... Cores>
bool WaitCore(const Timeout& timeout, Cores&... cores) noexcept {
  static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<BaseCore, Cores>), "Futures must be Future in Wait function");
  auto range = [&](auto&& func) noexcept {
    return (... + static_cast<std::size_t>(func(cores)));
  };
  return WaitRange<Event>(timeout, range, sizeof...(cores));
}

template <typename Event, typename Timeout, typename Iterator>
bool WaitIterator(const Timeout& timeout, Iterator it, std::size_t count) noexcept {
  static_assert(is_future_base_v<typename std::iterator_traits<Iterator>::value_type>,
                "Wait function Iterator must be point to some FutureBase");
  if (count == 0) {
    return true;
  }
  auto range = [&](auto&& func) noexcept {
    std::size_t wait_count = 0;
    std::conditional_t<std::is_same_v<Timeout, NoTimeoutTag>, Iterator&, Iterator> range_it = it;
    for (std::size_t i = 0; i != count; ++i) {
      wait_count += static_cast<std::size_t>(func(*range_it->GetCore()));
      ++range_it;
    }
    return wait_count;
  };
  return WaitRange<Event>(timeout, range, count);
}

extern template bool WaitCore<DefaultEvent, NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&) noexcept;

}  // namespace yaclib::detail

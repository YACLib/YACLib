#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/wait_core.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace yaclib::detail {

struct NoTimeoutTag {};

template <typename Timeout, typename Range>
bool WaitRange(const Timeout& t, const Range& range, std::size_t count) {
  AtomicCounter<WaitCore, WaitCoreDeleter> wait_core{count + 1};
  // wait_core ref counter = n + 1, it is optimization: we don't want to notify when return true immediately
  auto const wait_count = range([&](detail::BaseCore& core) {
    return core.Empty() && core.SetWait(wait_core);
  });
  if (wait_count == 0 || wait_core.SubEqual(count - wait_count + 1)) {
    return true;
  }
  std::unique_lock lock{wait_core.m};
  std::size_t reset_count = 0;
  if constexpr (!std::is_same_v<Timeout, NoTimeoutTag>) {
    // If you have problem with TSAN here, check this link: https://github.com/google/sanitizers/issues/1259
    // TLDR: new pthread function is not supported by thread sanitizer yet.
    if (wait_core.Wait(lock, t)) {
      return true;
    }
    reset_count = range([](detail::BaseCore& core) {
      return core.ResetWait();
    });
    if (reset_count != 0 && (reset_count == wait_count || wait_core.SubEqual(reset_count))) {
      return false;
    }
    // We know we have Result, but we must wait until wait_core was not used by cs
  }
  wait_core.Wait(lock);
  return reset_count == 0;
}

template <typename Timeout, typename... Cores>
bool WaitCores(const Timeout& t, Cores&... cs) {
  static_assert(sizeof...(cs) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<detail::BaseCore, Cores>), "Futures must be Future in Wait function");
  auto range = [&](auto&& functor) {
    return (... + static_cast<std::size_t>(functor(cs)));
  };
  return WaitRange(t, range, sizeof...(cs));
}

extern template bool WaitCores<NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);

template <typename Timeout, typename ValueIt, typename RangeIt>
bool WaitIters(const Timeout& t, ValueIt it, RangeIt begin, RangeIt end) {
  static_assert(is_future_v<typename std::iterator_traits<ValueIt>::value_type>,
                "Wait function Iterator must be point to some Future");
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Wait*(..., begin, distance(begin, end))
  auto const size = end - begin;
  if (size == 0) {
    return true;
  }
  auto range = [&](auto&& functor) {
    auto temp_it = it;
    auto temp_begin = begin;
    std::size_t count = 0;
    while (temp_begin != end) {
      count += static_cast<std::size_t>(functor(*temp_it->GetCore()));
      ++temp_it;
      ++temp_begin;
    }
    return count;
  };
  return WaitRange(t, range, size);
}

}  // namespace yaclib::detail

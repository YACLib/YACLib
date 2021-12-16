#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/wait_core.hpp>
#include <yaclib/util/counters.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

struct NoTimeoutTag {};

template <typename Timeout, typename Range>
bool WaitRange(const Timeout& t, Range range) {
  util::Counter<detail::WaitCore, detail::WaitCoreDeleter> wait_core;
  // wait_core ref counter = 1, it is optimization: we don't want to notify when return true immediately
  if (range([&](detail::BaseCore& core) {
        return core.SetWait(wait_core);
      })) {
    return true;
  }
  wait_core.DecRef();
  std::unique_lock guard{wait_core.m};
  bool ready{true};
  if constexpr (!std::is_same_v<Timeout, NoTimeoutTag>) {
    // If you have problem with TSAN here, check this link: https://github.com/google/sanitizers/issues/1259
    // TLDR: new pthread function is not supported by thread sanitizer yet.
    ready = wait_core.Wait(guard, t);
    if (ready) {
      return true;
    }
    guard.unlock();
    if (range([](detail::BaseCore& core) {
          return core.ResetWait();
        })) {
      return false;
    }
    // We know we have Result, but we must wait until wait_core was not used by cs
    guard.lock();
  }
  wait_core.Wait(guard);
  return ready;
}

template <typename Timeout, typename... Cores>
bool WaitCores(const Timeout& t, Cores&... cs) {
  static_assert(sizeof...(cs) >= 1, "Number of futures must be at least one");
  static_assert((... && std::is_same_v<detail::BaseCore, Cores>), "Futures must be Future in Wait function");
  auto range = [&](auto&& functor) {
    return (... & functor(cs));
  };
  return WaitRange(t, range);
}

extern template bool WaitCores<NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);

template <typename Timeout, typename ValueIt, typename RangeIt>
bool WaitIters(const Timeout& t, ValueIt it, RangeIt begin, RangeIt end) {
  static_assert(util::IsFutureV<typename std::iterator_traits<ValueIt>::value_type>,
                "Wait function Iterator must be point to some Future");
  if (begin == end) {
    return true;
  }
  auto range = [&](auto&& functor) {
    auto curr = it;
    auto first = begin;
    auto last = end;
    bool ok = true;
    for (; first != last; ++first) {
      ok &= functor(*curr->GetCore());
      ++curr;
    }
    return ok;
  };
  return WaitRange(t, range);
}

}  // namespace yaclib::detail

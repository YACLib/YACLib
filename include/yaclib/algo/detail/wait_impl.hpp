#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/wait_core.hpp>
#include <yaclib/util/counters.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

struct NoTimeoutTag {};

template <typename Timeout, typename... Cores>
bool Wait(const Timeout& t, Cores&... cs) {
  static_assert(sizeof...(cs) > 0, "Number of futures must be more than zero");
  static_assert((... && std::is_same_v<detail::BaseCore, Cores>), "Futures must be Future in Wait function");
  util::Counter<detail::WaitCore, detail::WaitCoreDeleter> wait_core;
  wait_core.IncRef();  // Optimization: we don't want to notify when return true immediately
  if ((... & cs.SetWait(wait_core))) {
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
    if ((... & cs.ResetWait())) {
      return false;
    }
    // We know we have Result, but we must wait until wait_core was not used by cs
    guard.lock();
  }
  wait_core.Wait(guard);
  return ready;
}

extern template bool Wait<NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);

}  // namespace yaclib::detail

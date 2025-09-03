#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/wait_event.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/default_event.hpp>
#include <yaclib/util/detail/set_deleter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace yaclib::detail {

struct NoTimeoutTag final {};

template <typename Event, typename Timeout, typename Range>
bool WaitRange(Event& event, const Timeout& timeout, Range&& range, std::size_t count) noexcept {
  const auto wait_count = [&] {
    if constexpr (Event::kShared) {
      return range([&, callback_count = std::size_t{}](auto handle) mutable noexcept {
        if constexpr (std::is_same_v<UniqueHandle, decltype(handle)>) {
          return handle.SetCallback(event.GetCall());
        } else {
          return handle.SetCallback(event.callbacks[callback_count++]);
        }
      });
    } else {
      return range([&](auto handle) noexcept {
        return handle.SetCallback(event.GetCall());
      });
    }
  }();

  if (wait_count == 0 || event.SubEqual(count - wait_count + 1)) {
    return true;
  }

  auto token = event.Make();
  std::size_t reset_count = 0;

  // Not available for shared future
  // but this is not always if-constexpred away in case of shared futures
  if constexpr (!std::is_same_v<Timeout, NoTimeoutTag>) {
    // If you have problem with TSAN here, check this link: https://github.com/google/sanitizers/issues/1259
    // TLDR: new pthread function is not supported by thread sanitizer yet.
    if (event.Wait(token, timeout)) {
      return true;
    }
    reset_count = range([](UniqueHandle handle) noexcept {
      return handle.Reset();
    });
    if (reset_count != 0 && (reset_count == wait_count || event.SubEqual(reset_count))) {
      return false;
    }
    // We know we have `wait_count - reset_count` Results, but we must wait until event was not used by cores
  }
  event.Wait(token);
  return reset_count == 0;  // LCOV_EXCL_LINE lcov won't parse it
}

template <typename Event, typename Timeout, typename... Handles>
bool WaitCore(const Timeout& timeout, Handles... handles) noexcept {
  static_assert(sizeof...(handles) >= 1, "Number of futures must be at least one");

  static constexpr size_t kSharedCount = Count<SharedHandle, Handles...>;
  static_assert(kSharedCount == 0 || std::is_same_v<Timeout, NoTimeoutTag>);

  auto range = [&](auto&& func) noexcept {
    return (... + static_cast<std::size_t>(func(handles)));
  };

  using CoreEvent = std::conditional_t<sizeof...(handles) == 1, MultiEvent<Event, OneCounter, CallCallback>,
                                       MultiEvent<Event, AtomicCounter, CallCallback>>;

  // If we have only one shared handle, we can use .next of the original event
  using FinalEvent = std::conditional_t<kSharedCount <= 1, CoreEvent, StaticSharedEvent<CoreEvent, kSharedCount>>;
  FinalEvent event{sizeof...(handles) + 1};

  return WaitRange(event, timeout, range, sizeof...(handles));
}

template <typename Event, typename Timeout, typename Iterator>
bool WaitIterator(const Timeout& timeout, Iterator it, std::size_t count) noexcept {
  static_assert(is_waitable_v<typename std::iterator_traits<Iterator>::value_type>,
                "Wait function Iterator must be point to some Waitable (Future or SharedFuture)");
  static constexpr bool kShared = std::is_same_v<decltype(it->GetHandle()), SharedHandle>;

  if (count == 0) {
    return true;
  }

  // TODO(ocelaiwo): We can try to unroll count more to avoid
  // dynamic allocation in most common cases
  if (count == 1) {
    YACLIB_ASSERT(it->Valid());
    return WaitCore<Event>(timeout, it->GetHandle());
  }

  auto range = [&](auto&& func) noexcept {
    std::size_t wait_count = 0;
    std::conditional_t<std::is_same_v<Timeout, NoTimeoutTag>, Iterator&, Iterator> range_it = it;
    for (std::size_t i = 0; i != count; ++i) {
      YACLIB_ASSERT(range_it->Valid());
      wait_count += static_cast<std::size_t>(func(range_it->GetHandle()));
      ++range_it;
    }
    return wait_count;
  };

  using CoreEvent = MultiEvent<Event, AtomicCounter, CallCallback>;
  using FinalEvent = std::conditional_t<kShared, DynamicSharedEvent<CoreEvent>, CoreEvent>;
  FinalEvent event{count + 1};

  return WaitRange(event, timeout, range, count);
}

extern template bool WaitCore<DefaultEvent, NoTimeoutTag, UniqueHandle>(const NoTimeoutTag&, UniqueHandle) noexcept;
extern template bool WaitCore<DefaultEvent, NoTimeoutTag, SharedHandle>(const NoTimeoutTag&, SharedHandle) noexcept;

}  // namespace yaclib::detail

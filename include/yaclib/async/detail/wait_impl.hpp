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

template <typename Event, size_t SharedCount, typename Timeout, typename Range>
bool WaitRange(const Timeout& timeout, Range&& range, std::size_t total_count) noexcept {
  static_assert(SharedCount == 0 || std::is_same_v<Timeout, NoTimeoutTag>);

  // Event ref counter = n + 1, it is optimization: we don't want to notify when return true immediately
  auto shared_event = [&] {
    if constexpr (SharedCount <= 1) {
      // If we have only one shared handle, we can use .next of the original event
      return Event{total_count + 1};
    } else {
      // One of the shared handles will use .next of the original event
      return StaticSharedEvent<Event, SharedCount - 1>{total_count + 1};
    }
  }();

  Event& event = [&]() -> Event& {
    if constexpr (SharedCount <= 1) {
      return shared_event;
    } else {
      return shared_event.event;
    }
  }();

  size_t callback_count = 0;

  const auto wait_count = [&] {
    if constexpr (SharedCount <= 1) {
      return range([&](auto handle) noexcept {
        return handle.SetCallback(event.GetCall());
      });
    } else {
      return range([&](auto handle) noexcept {
        if constexpr (std::is_same_v<UniqueHandle, decltype(handle)>) {
          return handle.SetCallback(event.GetCall());
        } else {
          if (callback_count == SharedCount - 1) {
            return handle.SetCallback(event.GetCall());
          } else {
            return handle.SetCallback(shared_event.callbacks[callback_count++]);
          }
        }
      });
    }
  }();

  // This does not compile
  // const auto wait_count = range([&](auto handle) noexcept {
  //   if constexpr (std::is_same_v<UniqueHandle, decltype(handle)> || SharedCount <= 1) {
  //     return handle.SetCallback(event.GetCall());
  //   } else {
  //     if (callback_count == SharedCount - 1) {
  //       return handle.SetCallback(event.GetCall());
  //     } else {
  //       return handle.SetCallback(shared_event.callbacks[callback_count++]);
  //     }
  //   }
  // });

  if (wait_count == 0 || event.SubEqual(total_count - wait_count + 1)) {
    return true;
  }

  auto token = event.Make();
  std::size_t reset_count = 0;

  // Not available for shared future
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
  return reset_count == 0;  // LCOV_EXCL_LINE shitty gcov cannot parse it
}

// Used only when waiting on an iterator over shared futures
// Shared futures do not support timeout so no timeout parameter
template <typename Event, typename Range>
bool WaitRangeSharedDynamic(Range&& range, std::size_t shared_count) noexcept {
  YACLIB_ASSERT(shared_count >= 2);
  // Event ref counter = n + 1, it is optimization: we don't want to notify when return true immediately
  // One of the shared handles will use .next of the original event
  DynamicSharedEvent<Event> shared_event{shared_count + 1, shared_count - 1};

  size_t callback_count = 0;
  const auto wait_count = range([&](SharedHandle handle) noexcept {
    if (callback_count == shared_count - 1) {
      return handle.SetCallback(shared_event.event.GetCall());
    } else {
      return handle.SetCallback(shared_event.callbacks[callback_count++]);
    }
  });

  if (wait_count == 0 || shared_event.event.SubEqual(shared_count - wait_count + 1)) {
    return true;
  }

  auto token = shared_event.event.Make();
  std::size_t reset_count = 0;

  shared_event.event.Wait(token);
  return reset_count == 0;  // LCOV_EXCL_LINE shitty gcov cannot parse it
}

template <typename Event, typename Timeout, typename... Handles>
bool WaitCore(const Timeout& timeout, Handles... handles) noexcept {
  static_assert(sizeof...(handles) >= 1, "Number of futures must be at least one");

  static constexpr size_t SharedCount = TypeCount<SharedHandle, Handles...>();
  static_assert(SharedCount == 0 || std::is_same_v<Timeout, NoTimeoutTag>);

  auto range = [&](auto&& func) noexcept {
    return (... + static_cast<std::size_t>(func(handles)));
  };
  using FinalEvent = std::conditional_t<sizeof...(handles) == 1, MultiEvent<Event, OneCounter, CallCallback>,
                                        MultiEvent<Event, AtomicCounter, CallCallback>>;

  return WaitRange<FinalEvent, SharedCount>(timeout, range, sizeof...(handles));
}

template <typename Event, typename Timeout, typename Iterator>
bool WaitIterator(const Timeout& timeout, Iterator it, std::size_t count) noexcept {
  static_assert(is_waitable_v<typename std::iterator_traits<Iterator>::value_type>,
                "Wait function Iterator must be point to some Waitable (Future or SharedFuture)");
  static constexpr bool Shared = std::is_same_v<decltype(it->GetBaseHandle()), SharedHandle>;

  if (count == 0) {
    return true;
  }
  if (count == 1) {
    auto range = [&](auto&& func) noexcept {
      YACLIB_ASSERT(it->Valid());
      return static_cast<std::size_t>(func(it->GetBaseHandle()));
    };
    // SharedCount template parameter here is actually irrelevant
    return WaitRange<MultiEvent<Event, OneCounter, CallCallback>, Shared ? 1 : 0>(timeout, range, 1);
  }
  auto range = [&](auto&& func) noexcept {
    std::size_t wait_count = 0;
    std::conditional_t<std::is_same_v<Timeout, NoTimeoutTag>, Iterator&, Iterator> range_it = it;
    for (std::size_t i = 0; i != count; ++i) {
      YACLIB_ASSERT(range_it->Valid());
      wait_count += static_cast<std::size_t>(func(range_it->GetBaseHandle()));
      ++range_it;
    }
    return wait_count;
  };
  // TODO(ocelaiwo): We can try to unroll count up to some value to avoid
  // dynamic allocation in most common cases
  if constexpr (Shared) {
    return WaitRangeSharedDynamic<MultiEvent<Event, AtomicCounter, CallCallback>>(range, count);
  } else {
    return WaitRange<MultiEvent<Event, AtomicCounter, CallCallback>, 0>(timeout, range, count);
  }
}

extern template bool WaitCore<DefaultEvent, NoTimeoutTag, UniqueHandle>(const NoTimeoutTag&, UniqueHandle) noexcept;
extern template bool WaitCore<DefaultEvent, NoTimeoutTag, SharedHandle>(const NoTimeoutTag&, SharedHandle) noexcept;

}  // namespace yaclib::detail

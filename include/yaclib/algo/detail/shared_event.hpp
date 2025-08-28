#pragma once

#include <yaclib/algo/detail/inline_core.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/coro.hpp>
#endif

#include <array>
#include <vector>

namespace yaclib::detail {

// Used to wait on multiple shared futures
// because shared cores enqueue callbacks intrusively
// so we need many inline cores (many .next pointers)
// This callback just dispatches to the Event's CallCallback
template <typename Event>
struct EventHelperCallback final : InlineCore {
  // Default ctor needed for use inside std::array
  EventHelperCallback() = default;
  EventHelperCallback(Event* event) : event(event) {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept {
    return event->GetCall().Here(caller);
  }

  // TODO(ocelaiwo): For now shared futures never do symmetric transfer
  // It is possile to do so if the last callback is to destroy the shared core
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {  // LCOV_EXCL_LINE
    return event->GetCall().Next(caller);                                                 // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
#endif

  Event* event;
};

template <typename Event, size_t SharedCount>
struct StaticSharedEvent {
  static_assert(SharedCount > 1);

  static inline constexpr bool Shared = true;
  using CoreEvent = Event;

  StaticSharedEvent(size_t total_count) : event{total_count} {
    callbacks.fill(&event);
  }

  Event event;
  std::array<EventHelperCallback<Event>, SharedCount> callbacks;
};

template <typename Event>
struct DynamicSharedEvent {
  static inline constexpr bool Shared = true;
  using CoreEvent = Event;

  DynamicSharedEvent(size_t total_count, size_t shared_count) : event{total_count}, callbacks{shared_count, &event} {
  }

  Event event;
  std::vector<EventHelperCallback<Event>> callbacks;
};

}  // namespace yaclib::detail

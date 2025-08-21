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
  EventHelperCallback() = default;
  EventHelperCallback(Event* event) : event(event) {
  }

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept {
    return static_cast<InlineCore&>(event->GetCall()).Here(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return static_cast<InlineCore&>(event->GetCall()).Next(caller);
  }
#endif

  Event* event;
};

template <typename Event, size_t SharedCount>
struct StaticSharedEvent {
  StaticSharedEvent(size_t total_count) : event{total_count} {
    callbacks.fill(&event);
  }

  Event event;
  std::array<EventHelperCallback<Event>, SharedCount> callbacks;
};

template <typename Event>
struct DynamicSharedEvent {
  DynamicSharedEvent(size_t total_count, size_t shared_count) : event{total_count}, callbacks{shared_count, &event} {
  }

  Event event;
  std::vector<EventHelperCallback<Event>> callbacks;
};

}  // namespace yaclib::detail

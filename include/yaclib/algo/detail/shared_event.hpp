#pragma once

#include <yaclib/algo/detail/base_core.hpp>
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
  EventHelperCallback(Event* event) : event{event} {
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
struct StaticSharedEvent : public Event {
  static constexpr bool kShared = true;

  explicit StaticSharedEvent(size_t total_count) : Event{total_count} {
    callbacks.fill(this);
  }

  std::array<EventHelperCallback<Event>, SharedCount> callbacks;
};

template <typename Event>
struct DynamicSharedEvent : public Event {
  static constexpr bool kShared = true;

  explicit DynamicSharedEvent(size_t total_count) : Event{total_count}, callbacks{total_count - 1, this} {
  }

  std::vector<EventHelperCallback<Event>> callbacks;
};

template <typename Event, typename... Handles>
void SetCallbacksStatic(Event& event, Handles... handles) {
  const auto wait_count = [&] {
    if constexpr (!Event::kShared) {
      auto setter = [&](auto handle) {
        return handle.SetCallback(event);
      };
      return (... + static_cast<std::size_t>(setter(handles)));
    } else {
      auto setter = [&, callback_count = std::size_t{}](auto handle) mutable {
        if constexpr (std::is_same_v<decltype(handle), UniqueHandle>) {
          return handle.SetCallback(event);
        } else {
          return handle.SetCallback(event.callbacks[callback_count++]);
        }
      };
      return (... + static_cast<std::size_t>(setter(handles)));
    }
  }();

  event.count.fetch_sub(sizeof...(handles) - wait_count, std::memory_order_relaxed);
}

template <typename Event, typename It>
void SetCallbacksDynamic(Event& event, It it, std::size_t count) {
  std::size_t wait_count = 0;
  for (std::size_t i = 0; i != count; ++i) {
    YACLIB_ASSERT(it->Valid());
    if constexpr (std::is_same_v<decltype(it->GetHandle()), UniqueHandle>) {
      wait_count += static_cast<std::size_t>(it->GetHandle().SetCallback(event));
    } else {
      wait_count += static_cast<std::size_t>(it->GetHandle().SetCallback(event.callbacks[i]));
    }
    ++it;
  }
  event.count.fetch_sub(count - wait_count, std::memory_order_relaxed);
}

}  // namespace yaclib::detail

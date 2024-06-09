#pragma once

#include <yaclib/algo/detail/wait_event.hpp>
#include <yaclib/algo/one_shot_event.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/set_deleter.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>

namespace yaclib {

/**
 * An object that allows you to Add some amount of async operations and then Wait for it to be Done
 */
template <typename Event = OneShotEvent>
class WaitGroup final {
 public:
  explicit WaitGroup(std::size_t count = 0) noexcept : _event{count} {
  }

  /**
   * Add some amount of async operations
   *
   * Can be called parallel with Add, Done,
   * and with Wait, but only if you call it when some Add not Done yet
   *
   * \param count of async operations
   */
  YACLIB_INLINE void Add(std::size_t count = 1) noexcept {
    _event.Add(count);
  }

  /**
   * Done some Add-ed async operations
   *
   * \param count of async operations
   */
  YACLIB_INLINE void Done(std::size_t count = 1) noexcept {
    _event.Sub(count);
  }

  /**
   * Consume \ref Future by WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add before call Consume
   * \param futures to wait
   */
  template <bool NeedAdd = true, typename... V, typename... E>
  YACLIB_INLINE void Consume(FutureBase<V, E>&&... futures) noexcept {
    InsertCore<true, NeedAdd>(UpCast<detail::BaseCore>(*futures.GetCore().Release())...);
  }

  /**
   * Consume \ref Future by WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add before call Consume
   * \param begin iterator to futures to Add
   * \param end iterator to futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> Consume(It begin, It end) noexcept {
    InsertIt<true, NeedAdd>(begin, static_cast<std::size_t>(end - begin));
  }

  /**
   * Consume \ref Future by WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add before call Consume
   * \param begin iterator to futures to Add
   * \param count count of futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE void Consume(It begin, std::size_t count) noexcept {
    InsertIt<true, NeedAdd>(begin, count);
  }

  /**
   * Attach \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add before call Attach
   * \param futures to wait
   */
  template <bool NeedAdd = true, typename... V, typename... E>
  YACLIB_INLINE void Attach(FutureBase<V, E>&... futures) noexcept {
    InsertCore<false, NeedAdd>(UpCast<detail::BaseCore>(*futures.GetCore())...);
  }

  /**
   * Attach \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add before call Attach
   * \param begin iterator to futures to Add
   * \param end iterator to futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> Attach(It begin, It end) noexcept {
    InsertIt<false, NeedAdd>(begin, static_cast<std::size_t>(end - begin));
  }

  /**
   * Attach \ref Future to WaitGroup with auto Done
   *
   * Also \see Add
   *
   * \tparam NeedAdd if true make implicit Add, if false you should make explicit Add before call Attach
   * \param begin iterator to futures to Add
   * \param count count of futures to Add
   */
  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE void Attach(It begin, std::size_t count) noexcept {
    InsertIt<false, NeedAdd>(begin, count);
  }

  /**
   * TODO
   */
  YACLIB_INLINE void Wait() noexcept {
    _event.Wait();
  }
  /**
   * TODO
   */
  template <typename Rep, typename Period>
  YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration) {
    return _event.WaitFor(timeout_duration);
  }

  /**
   * TODO
   */
  template <typename Clock, typename Duration>
  YACLIB_INLINE bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return _event.WaitUntil(timeout_time);
  }

#if YACLIB_CORO != 0
  /**
   * See OneShotEvent::Await
   */
  YACLIB_INLINE auto Await() noexcept {
    return _event.Await();
  }

  /**
   * See OneShotEvent::Await
   */
  YACLIB_INLINE auto AwaitSticky() noexcept {
    return _event.AwaitSticky();
  }

  /**
   * See OneShotEvent::AwaitOn
   */
  YACLIB_INLINE auto AwaitOn(IExecutor& e) noexcept {
    return _event.AwaitOn(e);
  }

  /**
   * just shortcut for co_await wait_group.Await();
   *
   * TODO(MBkkt) move all shortcut to AwaitSticky
   */
  YACLIB_INLINE auto operator co_await() noexcept {
    return Await();
  }
#endif

  /**
   * Reinitializes WaitGroup, semantically the same as `*this = {};`
   *
   * If you don't explicitly call this method,
   * then after the first one, Wait will always return immediately.
   *
   * \note Not thread-safe
   */
  YACLIB_INLINE void Reset(std::size_t count = 0) noexcept {
    _event.Reset();
    _event.count.store(count, std::memory_order_relaxed);
  }

 private:
  template <bool NeedMove, bool NeedAdd, typename... Cores>
  YACLIB_INLINE void InsertCore(Cores&... cores) noexcept {
    static_assert(sizeof...(cores) >= 1, "Number of futures must be at least one");
    static_assert((... && std::is_same_v<detail::BaseCore, Cores>),
                  "Futures must be Future in WaitGroup::Consume/Attach function");
    auto range = [&](auto&& func) noexcept {
      return (... + static_cast<std::size_t>(func(cores)));
    };
    InsertRange<NeedMove, NeedAdd>(range, sizeof...(cores));
  }

  template <bool NeedMove, bool NeedAdd, typename It>
  YACLIB_INLINE void InsertIt(It it, std::size_t count) noexcept {
    static_assert(is_future_base_v<typename std::iterator_traits<It>::value_type>,
                  "WaitGroup::Consume/Attach function Iterator must be point to some Future");
    if (count == 0) {
      return;
    }
    auto range = [&](auto&& func) noexcept {
      std::size_t wait_count = 0;
      for (std::size_t i = 0; i != count; ++i) {
        if constexpr (NeedMove) {
          wait_count += static_cast<std::size_t>(func(*it->GetCore().Release()));
        } else {
          wait_count += static_cast<std::size_t>(func(*it->GetCore()));
        }
        ++it;
      }
      return wait_count;
    };
    InsertRange<NeedMove, NeedAdd>(range, count);
  }

  template <bool NeedMove, bool NeedAdd, typename Range>
  void InsertRange(const Range& range, std::size_t count) noexcept {
    if constexpr (NeedAdd) {
      Add(count);
    }
    const auto wait_count = range([&](detail::BaseCore& core) noexcept {
      if constexpr (NeedMove) {
        if (core.SetCallback(_event.GetDrop())) {
          return true;
        }
        core.DecRef();
        return false;
      } else {
        return core.SetCallback(_event.GetCall());
      }
    });
    if (count != wait_count) {  // TODO(MBkkt) is it necessary?
      Done(count - wait_count);
    }
  }
  detail::MultiEvent<Event, detail::AtomicCounter, detail::CallCallback, detail::DropCallback> _event;
};

extern template class WaitGroup<OneShotEvent>;

}  // namespace yaclib

#pragma once

#include <yaclib/async/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/default_event.hpp>

#include <cstddef>

namespace yaclib {

/**
 * Wait until \ref Ready becomes true
 *
 * \param fs one or more futures to wait
 */
template <typename Event = detail::DefaultEvent, typename... Waited>
YACLIB_INLINE std::enable_if_t<(... && is_waitable_v<Waited>), void> Wait(Waited&... fs) noexcept {
  YACLIB_ASSERT(... && fs.Valid());
  detail::WaitCore<Event>(detail::NoTimeoutTag{}, *fs.GetBaseCore()...);
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin iterator to futures to wait
 * \param end iterator to futures to wait
 */
template <typename Event = detail::DefaultEvent, typename It>
YACLIB_INLINE std::enable_if_t<is_waitable_v<typename std::iterator_traits<It>::reference>, void> Wait(
  It begin, It end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Wait*(..., begin, distance(begin, end))
  detail::WaitIterator<Event>(detail::NoTimeoutTag{}, begin, static_cast<std::size_t>(end - begin));
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin iterator to futures to wait
 * \param count of futures to wait
 */
template <typename Event = detail::DefaultEvent, typename It>
YACLIB_INLINE std::enable_if_t<is_waitable_v<typename std::iterator_traits<It>::reference>, void> Wait(
  It begin, std::size_t count) noexcept {
  detail::WaitIterator<Event>(detail::NoTimeoutTag{}, begin, count);
}

}  // namespace yaclib

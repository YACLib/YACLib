#pragma once

#include <yaclib/algo/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>

#include <cstddef>

namespace yaclib {

/**
 * Wait until \ref Ready becomes true
 *
 * \param fs one or more futures to wait
 */
template <typename Event = detail::DefaultEvent, typename... V, typename... E>
void Wait(Future<V, E>&... fs) {
  detail::WaitCore<Event>(detail::NoTimeoutTag{}, static_cast<detail::BaseCore&>(*fs.GetCore())...);
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin Iterator to futures to wait
 * \param end Iterator to futures to wait
 */
template <typename Event = detail::DefaultEvent, typename It>
std::enable_if_t<!is_future_v<It>, void> Wait(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Wait*(..., begin, distance(begin, end))
  detail::WaitIterator<Event>(detail::NoTimeoutTag{}, begin, end - begin);
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin Iterator to futures to wait
 * \param size count of futures to wait
 */
template <typename Event = detail::DefaultEvent, typename It>
void Wait(It begin, std::size_t size) {
  detail::WaitIterator<Event>(detail::NoTimeoutTag{}, begin, size);
}

}  // namespace yaclib

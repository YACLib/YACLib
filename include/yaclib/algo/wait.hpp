#pragma once

#include <yaclib/algo/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>

#include <cstddef>

namespace yaclib {

/**
 * Wait until \ref Ready becomes true
 *
 * \param fs one or more futures to wait
 */
template <typename... V, typename... E>
void Wait(Future<V, E>&... fs) {
  detail::WaitCores(detail::NoTimeoutTag{}, static_cast<detail::BaseCore&>(*fs.GetCore())...);
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin Iterator to futures to wait
 * \param end Iterator to futures to wait
 */
template <typename Iterator>
std::enable_if_t<!is_future_v<Iterator>, void> Wait(Iterator begin, Iterator end) {
  detail::WaitIters(detail::NoTimeoutTag{}, begin, begin, end);
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin Iterator to futures to wait
 * \param size count of futures to wait
 */
template <typename Iterator>
void Wait(Iterator begin, std::size_t size) {
  detail::WaitIters(detail::NoTimeoutTag{}, begin, std::size_t{0}, size);
}

}  // namespace yaclib

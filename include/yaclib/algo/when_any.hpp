#pragma once

#include <yaclib/algo/detail/when_any_impl.hpp>

namespace yaclib {

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , size the range of futures to combine
 * \return Future<T>
 */
template <WhenPolicy P = WhenPolicy::LastFail, typename It,
          typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
Future<T> WhenAny(It begin, size_t size) {
  return detail::WhenAnyImpl<P, T>(begin, size_t{0}, size);
}

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , end the range of futures to combine
 * \return Future<T>
 */
template <WhenPolicy P = WhenPolicy::LastFail, typename It,
          typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
Future<T> WhenAny(It begin, It end) {
  return detail::WhenAnyImpl<P, T>(begin, begin, end);
}

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam T type of all passed futures
 * \param head , tail one or more futures to combine
 * \return Future<T>
 */
template <WhenPolicy P = WhenPolicy::LastFail, typename T, typename... Ts>
Future<T> WhenAny(Future<T>&& head, Future<Ts>&&... tail) {
  constexpr size_t kSize = 1 + sizeof...(Ts);
  static_assert(kSize >= 2, "WhenAny wants at least two futures");
  auto [future, combinator] = detail::AnyCombinator<T, P>::Make(kSize);
  detail::WhenAnyImpl<P>(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib

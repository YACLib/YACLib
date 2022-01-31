#pragma once

#include <yaclib/algo/detail/when_all_impl.hpp>

namespace yaclib {

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , size the range of futures to combine
 * \return Future<std::vector<T>>
 */
template <WhenPolicy P = WhenPolicy::FirstFail, typename It,
          typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
auto WhenAll(It begin, size_t size) {
  static_assert(P == WhenPolicy::FirstFail, "TODO(Ri7ay) Add other policy for WhenAll");
  auto [future, combinator] = detail::AllCombinator<T>::Make(size);
  for (size_t i = 0; i != size; ++i) {
    begin->GetCore()->SetCallbackInline(combinator);
    std::move(*begin).Detach();
    ++begin;
  }
  return std::move(future);
}

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \param begin , end the range of futures to combine
 * \return Future<std::vector<T>>
 */
template <WhenPolicy P = WhenPolicy::FirstFail, typename It>
auto WhenAll(It begin, It end) {
  static_assert(P == WhenPolicy::FirstFail, "TODO(Ri7ay) Add other policy for WhenAll");
  static_assert(util::IsFutureV<typename std::iterator_traits<It>::value_type>,
                "When function Iterator must be point to some Future");
  return WhenAll(begin, std::distance(begin, end));
}

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam T type of all passed futures
 * \param head , tail one or more futures to combine
 * \return Future<std::array<T>>
 */
template <WhenPolicy P = WhenPolicy::FirstFail, typename T, typename... Ts>
auto WhenAll(Future<T>&& head, Future<Ts>&&... tail) {
  static_assert(P == WhenPolicy::FirstFail, "TODO(Ri7ay) Add other policy for WhenAll");
  constexpr size_t kSize = 1 + sizeof...(Ts);
  static_assert(kSize >= 2, "WhenAll wants at least two futures");
  auto [future, combinator] = detail::AllCombinator<T, kSize>::Make(1);
  detail::WhenAllImpl<kSize>(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib

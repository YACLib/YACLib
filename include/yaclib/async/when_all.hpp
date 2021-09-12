#pragma once

#include <yaclib/async/run.hpp>

#include <array>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace yaclib::async {
namespace detail {

template <typename T>
class AllCombinatorBase {
 protected:
  alignas(kCacheLineSize) std::atomic<bool> _done{false};
  alignas(kCacheLineSize) std::atomic<size_t> _ticket{0};
  T _results;
};

template <>
class AllCombinatorBase<void> {
 protected:
  alignas(kCacheLineSize) std::atomic<bool> _done{false};
};

template <
    typename T, size_t N = std::numeric_limits<size_t>::max(), bool IsArray = (N != std::numeric_limits<size_t>::max()),
    typename FutureValue =
        std::conditional_t<std::is_void_v<T>, void, std::conditional_t<IsArray, std::array<T, N>, std::vector<T>>>>
class AllCombinator : public AllCombinatorBase<FutureValue>, public IRef {
  using Base = AllCombinatorBase<FutureValue>;

 public:
  static std::pair<Future<FutureValue>, container::intrusive::Ptr<AllCombinator>> Make(size_t size = 0) {
    auto [future, promise] = MakeContract<FutureValue>();
    if constexpr (!IsArray) {
      if (size == 0) {
        if constexpr (std::is_void_v<T>) {
          std::move(promise).Set();
        } else {
          std::move(promise).Set(std::vector<T>{});
        }
        return {std::move(future), nullptr};
      }
    }
    return {std::move(future), new container::Counter<AllCombinator>{std::move(promise), size}};
  }

  void Combine(util::Result<T>&& result) noexcept(std::is_void_v<T> || std::is_nothrow_move_assignable_v<T>) {
    if (Base::_done.load(std::memory_order_acquire)) {
      return;
    }
    auto state = result.State();
    if (state == util::ResultState::Value) {
      if constexpr (!std::is_void_v<T>) {
        const auto ticket = AllCombinatorBase<FutureValue>::_ticket.fetch_add(1, std::memory_order_acq_rel);
        AllCombinatorBase<FutureValue>::_results[ticket] = std::move(result).Value();
      }
      return;
    }
    if (Base::_done.exchange(true, std::memory_order_acq_rel)) {
      return;
    }
    if (state == util::ResultState::Error) {
      std::move(_promise).Set(std::move(result).Error());
    } else {
      std::move(_promise).Set(std::move(result).Exception());
    }
  }

  ~AllCombinator() {
    if (!Base::_done.load(std::memory_order_acquire)) {
      if constexpr (!std::is_void_v<T>) {
        std::move(_promise).Set(std::move(AllCombinatorBase<FutureValue>::_results));
      } else {
        std::move(_promise).Set();
      }
    }
  }

 private:
  explicit AllCombinator(Promise<FutureValue> promise, [[maybe_unused]] size_t size = 0)
      : _promise{std::move(promise)} {
    if constexpr (!std::is_void_v<T> && !IsArray) {
      AllCombinatorBase<FutureValue>::_results.resize(size);
    }
  }

  Promise<FutureValue> _promise;
};

template <size_t N, typename T, typename... Fs>
void WhenAllImpl(container::intrusive::Ptr<AllCombinator<T, N>>& combinator, Future<T>&& head, Fs&&... tail) {
  std::move(head).Subscribe([c = combinator](util::Result<T>&& result) mutable {
    c->Combine(std::move(result));
    c = nullptr;
  });
  if constexpr (sizeof...(tail) != 0) {
    WhenAllImpl(combinator, std::forward<Fs>(tail)...);
  }
}

}  // namespace detail

/**
 * \brief Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin, size the range of futures to combine
 * \return Future<vector<T>>
 */
template <typename It, typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
auto WhenAll(It begin, size_t size) {
  auto [future, combinator] = detail::AllCombinator<T>::Make(size);
  for (size_t i = 0; i != size; ++i) {
    std::move(*begin).Subscribe([c = combinator](util::Result<T>&& result) mutable {
      c->Combine(std::move(result));
      c = nullptr;
    });
    ++begin;
  }
  return std::move(future);
}

/**
 * \brief Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin, end the range of futures to combine
 * \return Future<vector<T>>
 */
template <typename It>
auto WhenAll(It begin, It end) {
  static_assert(util::IsFutureV<typename std::iterator_traits<It>::value_type>);
  return WhenAll(begin, std::distance(begin, end));
}

/**
 * \brief Create \ref Future which will be ready when all futures are ready
 *
 * \tparam T type of all passed futures
 * \param head, tail one or more futures to combine
 * \return Future<array<T>>
 */
template <typename T, typename... Fs>
auto WhenAll(Future<T>&& head, Fs&&... tail) {
  static_assert((... && util::IsFutureV<Fs>));  // TODO(kononovk): Add static assert that FutureValue<Fs> = T
  auto [future, combinator] = detail::AllCombinator<T, sizeof...(Fs) + 1>::Make();
  detail::WhenAllImpl<sizeof...(Fs) + 1>(combinator, std::move(head), std::forward<Fs>(tail)...);
  return std::move(future);
}

}  // namespace yaclib::async

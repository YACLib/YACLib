#pragma once

#include <yaclib/async/run.hpp>

#include <array>
#include <type_traits>
#include <vector>

namespace yaclib::async {
namespace detail {

template <typename T>
class AllCombinatorBase {
 protected:
  T _results;
  alignas(kCacheLineSize) std::atomic<size_t> _ticket{0};
};

template <>
class AllCombinatorBase<void> {};

template <
    typename T, size_t N = std::numeric_limits<size_t>::max(), bool IsArray = (N != std::numeric_limits<size_t>::max()),
    typename FutureValue =
        std::conditional_t<std::is_void_v<T>, void, std::conditional_t<IsArray, std::array<T, N>, std::vector<T>>>>
class AllCombinator : public AllCombinatorBase<FutureValue>, public IRef {
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

  void Combine(util::Result<T> result) noexcept(std::is_void_v<T> || std::is_nothrow_move_assignable_v<T>) {
    if (_done.load(std::memory_order_acquire)) {
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
    if (_done.exchange(true, std::memory_order_acq_rel)) {
      return;
    }
    if (state == util::ResultState::Error) {
      std::move(_promise).Set(std::move(result).Error());
    } else {
      std::move(_promise).Set(std::move(result).Exception());
    }
  }

  ~AllCombinator() {
    if (!_done.load(std::memory_order_acquire)) {
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

  alignas(kCacheLineSize) std::atomic<bool> _done;
  Promise<FutureValue> _promise;
};

template <size_t N, typename T, typename... Fs>
void WhenAllImpl(container::intrusive::Ptr<AllCombinator<T, N>>& combinator, Future<T>&& head, Fs&&... tail) {
  std::move(head).Subscribe([c = combinator](util::Result<T> result) mutable {
    c->Combine(std::move(result));
  });
  if constexpr (sizeof...(tail) != 0) {
    WhenAllImpl(combinator, std::move(tail)...);
  }
}

}  // namespace detail

template <typename It, typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
auto WhenAll(It begin, It end, size_t size) {
  auto [future, combinator] = detail::AllCombinator<T>::Make(size);
  for (; begin != end; ++begin) {
    std::move(*begin).Subscribe([c = combinator](util::Result<T> result) mutable {
      c->Combine(std::move(result));
    });
  }
  return std::move(future);
}

template <typename It>
auto WhenAll(It begin, It end) {
  static_assert(util::IsFutureV<typename std::iterator_traits<It>::value_type>);
  return WhenAll(begin, end, std::distance(begin, end));
}

template <typename T, typename... Fs>
auto WhenAll(Future<T>&& head, Fs&&... tail) {
  static_assert((... && util::IsFutureV<Fs>));  // TODO(kononovk): Add static assert that FutureValue<Fs> = T
  auto [future, combinator] = detail::AllCombinator<T, sizeof...(Fs) + 1>::Make();
  detail::WhenAllImpl<sizeof...(Fs) + 1>(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib::async

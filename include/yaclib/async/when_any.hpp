#pragma once

#include <yaclib/async/run.hpp>

#include <array>
#include <iostream>
#include <type_traits>
#include <vector>

namespace yaclib::async {
namespace detail {

template <typename T, size_t N = std::numeric_limits<size_t>::max(),
          typename FutureValue = std::conditional_t<std::is_void_v<T>, void, T>>
class AnyCombinator : public IRef {
 public:
  static std::pair<Future<FutureValue>, container::intrusive::Ptr<AnyCombinator>> Make(size_t size = 0) {
    auto [future, promise] = MakeContract<T>();
    if (size == 0) {
      if constexpr (std::is_void_v<T>) {
        std::move(promise).Set();
      } else {
        // TODO: what should be return value?
        std::move(promise).Set(util::Result<T>{});
      }
      return {std::move(future), nullptr};
    }
    return {std::move(future), new container::Counter<AnyCombinator<T>>{std::move(promise)}};
  }

  void Combine(util::Result<T> result) {
    if (_done.load(std::memory_order_acquire)) {
      return;
    }

    if (_done.exchange(true, std::memory_order_acq_rel)) {
      return;
    }
    std::move(_promise).Set(std::move(result));
  }

  AnyCombinator(Promise<T> promise) : _done{false}, _promise{std::move(promise)} {
  }

 private:
  alignas(kCacheLineSize) std::atomic<bool> _done;
  Promise<T> _promise;
};

template <typename T>
using AnyCombinatorPtr = container::intrusive::Ptr<AnyCombinator<T>>;

template <typename T, typename... Fs>
void WhenAnyImpl(detail::AnyCombinatorPtr<T>& combinator, Future<T>&& head, Fs&&... tail) {
  std::move(head).Subscribe([c = combinator](util::Result<T> result) mutable {
    c->Combine(std::move(result));
    c = nullptr;
  });
  if constexpr (sizeof...(tail) != 0) {
    WhenAnyImpl(combinator, std::move(tail)...);
  }
}

}  // namespace detail

template <typename It, typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
Future<T> WhenAny(It begin, It end) {
  auto [future, combinator] = detail::AnyCombinator<T>::Make(std::distance(begin, end));
  for (; begin != end; ++begin) {
    std::move(*begin).Subscribe([c = combinator](util::Result<T> result) mutable {
      c->Combine(std::move(result));
      c = nullptr;
    });
  }
  return std::move(future);
}

template <typename T, typename... Fs>
Future<T> WhenAny(Future<T>&& head, Fs&&... tail) {
  static_assert((... && util::IsFutureV<Fs>));
  auto [future, combinator] = detail::AnyCombinator<T>::Make(1);
  detail::WhenAnyImpl(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib::async

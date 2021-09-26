#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace yaclib::algo {

enum class PolicyWhenAny {
  FirstError,
  LastError,
};

namespace detail {

template <typename T, PolicyWhenAny P>
class AnyCombinator : public util::IRef {
 public:
  static std::pair<async::Future<T>, util::Ptr<AnyCombinator>> Make(bool empty = true) {
    auto [future, promise] = async::MakeContract<T>();
    if (empty) {
      std::move(promise).Set(util::Result<T>{});
      return {std::move(future), nullptr};
    }
    return {std::move(future), new util::Counter<AnyCombinator<T, P>>{std::move(promise)}};
  }

  void Combine(util::Result<T>&& result) {
    if (_done.load(std::memory_order_acquire)) {
      return;
    }

    if (result) {
      if (!_done.exchange(true, std::memory_order_acq_rel)) {
        std::move(_promise).Set(std::move(result));
      }
    } else if (!_error.load(std::memory_order_acquire) && !_error.exchange(true, std::memory_order_acq_rel)) {
      _except_error = std::move(result);
    }
  }

  ~AnyCombinator() override {
    if (!_done.load(std::memory_order_acquire)) {
      std::move(_promise).Set(std::move(_except_error));
    }
  }

 private:
  explicit AnyCombinator(async::Promise<T> promise) : _promise{std::move(promise)} {
  }

  alignas(kCacheLineSize) std::atomic<bool> _done{false};
  alignas(kCacheLineSize) std::atomic<bool> _error{false};
  util::Result<T> _except_error;
  async::Promise<T> _promise;
};

template <typename T>
class AnyCombinator<T, PolicyWhenAny::LastError> : public util::IRef {
 public:
  static std::pair<async::Future<T>, util::Ptr<AnyCombinator>> Make(size_t size = 0) {
    auto [future, promise] = async::MakeContract<T>();
    if (size == 0) {
      std::move(promise).Set(util::Result<T>{});
      return {std::move(future), nullptr};
    }
    return {std::move(future), new util::Counter<AnyCombinator<T, PolicyWhenAny::LastError>>{std::move(promise), size}};
  }

  void Combine(util::Result<T>&& result) {
    if (_done.load(std::memory_order_acquire)) {
      return;
    }

    if (result) {
      if (!_done.exchange(true, std::memory_order_acq_rel)) {
        std::move(_promise).Set(std::move(result));
      }
    } else if (_size.fetch_sub(1, std::memory_order_acq_rel) == 1U) {
      std::move(_promise).Set(std::move(result));
    }
  }

 private:
  explicit AnyCombinator(async::Promise<T> promise, size_t size = 0) : _size{size}, _promise{std::move(promise)} {
  }

  alignas(kCacheLineSize) std::atomic<bool> _done{false};
  alignas(kCacheLineSize) std::atomic<size_t> _size;
  async::Promise<T> _promise;
};

template <typename T, PolicyWhenAny P>
using AnyCombinatorPtr = util::Ptr<AnyCombinator<T, P>>;

template <PolicyWhenAny P = PolicyWhenAny::FirstError, typename T, typename... Fs>
void WhenAnyImpl(detail::AnyCombinatorPtr<T, P>& combinator, async::Future<T>&& head, Fs&&... tail) {
  std::move(head).Subscribe([c = combinator](util::Result<T>&& result) mutable {
    c->Combine(std::move(result));
    c = nullptr;
  });
  if constexpr (sizeof...(tail) != 0) {
    WhenAnyImpl(combinator, std::forward<Fs>(tail)...);
  }
}

}  // namespace detail

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin, size the range of futures to combine
 * \return Future<T>
 */
template <PolicyWhenAny P = PolicyWhenAny::FirstError, typename It,
          typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
auto WhenAny(It begin, size_t size) {
  auto [future, combinator] = [&size] {
    if constexpr (P == PolicyWhenAny::FirstError) {
      return detail::AnyCombinator<T, P>::Make(size == 0);
    } else {
      return detail::AnyCombinator<T, P>::Make(size);
    }
  }();
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
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin, end the range of futures to combine
 * \return Future<T>
 */
template <PolicyWhenAny P = PolicyWhenAny::FirstError, typename It,
          typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>>
auto WhenAny(It begin, It end) {
  auto [future, combinator] = [&begin, &end] {
    if constexpr (P == PolicyWhenAny::FirstError) {
      return detail::AnyCombinator<T, P>::Make(begin == end);
    } else {
      return detail::AnyCombinator<T, P>::Make(std::distance(begin, end));
    }
  }();
  for (; begin != end; ++begin) {
    std::move(*begin).Subscribe([c = combinator](util::Result<T>&& result) mutable {
      c->Combine(std::move(result));
      c = nullptr;
    });
  }
  return std::move(future);
}

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam T type of all passed futures
 * \param head, tail one or more futures to combine
 * \return Future<T>
 */
template <PolicyWhenAny P = PolicyWhenAny::FirstError, typename T, typename... Fs>
auto WhenAny(async::Future<T>&& head, Fs&&... tail) {
  static_assert((... && util::IsFutureV<Fs>));
  auto [future, combinator] = [] {
    if constexpr (P == PolicyWhenAny::FirstError) {
      return detail::AnyCombinator<T, P>::Make(false);
    } else {
      return detail::AnyCombinator<T, P>::Make(sizeof...(Fs) + 1);
    }
  }();
  detail::WhenAnyImpl<P>(combinator, std::move(head), std::forward<Fs>(tail)...);
  return std::move(future);
}

}  // namespace yaclib::algo

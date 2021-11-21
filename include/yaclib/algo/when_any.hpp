#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace yaclib {

enum class PolicyWhenAny {
  FirstError,
  LastError,
};

namespace detail {

template <typename T, PolicyWhenAny P>
class AnyCombinatorBase {
 protected:
  std::atomic<size_t> _size{0};
  Promise<T> _promise;

  AnyCombinatorBase(Promise<T> promise, size_t size) : _size(size), _promise{std::move(promise)} {
  }

  void CombineError(util::Result<T>&& result) {
    if (_size.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      std::move(_promise).Set(std::move(result));
    }
  };
};

template <typename T>
class AnyCombinatorBase<T, PolicyWhenAny::FirstError> {
 protected:
  std::atomic<bool> _error{false};
  util::Result<T> _except_error;
  Promise<T> _promise;

  AnyCombinatorBase(Promise<T> promise, size_t) : _promise{std::move(promise)} {
  }

  void CombineError(util::Result<T>&& result) {
    if (!_error.load(std::memory_order_acquire) && !_error.exchange(true, std::memory_order_acq_rel)) {
      _except_error = std::move(result);
    }
  }
};

template <typename T, PolicyWhenAny P>
class AnyCombinator : public BaseCore, public AnyCombinatorBase<T, P> {
  using Base = AnyCombinatorBase<T, P>;

 public:
  static std::pair<Future<T>, util::Ptr<AnyCombinator>> Make(size_t size) {
    auto [future, promise] = MakeContract<T>();
    if (size == 0) {
      std::move(promise).Set(util::Result<T>{});
      return {std::move(future), nullptr};
    }
    return {std::move(future), new util::Counter<AnyCombinator<T, P>>{std::move(promise), size}};
  }

  void CallInline(void* context) noexcept final {
    if (BaseCore::GetState() != BaseCore::State::HasStop) {
      Combine(std::move(static_cast<ResultCore<T>*>(context)->Get()));
    }
  }

  void Combine(util::Result<T>&& result) {
    if (_done.load(std::memory_order_acquire)) {
      return;
    }

    if (result) {
      if (!_done.exchange(true, std::memory_order_acq_rel)) {
        std::move(Base::_promise).Set(std::move(result));
      }
    } else {
      Base::CombineError(std::move(result));
    }
  }

  ~AnyCombinator() override {
    if constexpr (P == PolicyWhenAny::FirstError) {
      if (!_done.load(std::memory_order_acquire)) {
        std::move(Base::_promise).Set(std::move(Base::_except_error));
      }
    }
  }

 private:
  explicit AnyCombinator(Promise<T> promise, size_t size)
      : BaseCore{BaseCore::State::Empty}, Base{std::move(promise), size} {
  }

  std::atomic<bool> _done{false};
};

template <typename T, PolicyWhenAny P>
using AnyCombinatorPtr = util::Ptr<AnyCombinator<T, P>>;

template <PolicyWhenAny P = PolicyWhenAny::FirstError, typename T, typename... Fs>
void WhenAnyImpl(detail::AnyCombinatorPtr<T, P>& combinator, Future<T>&& head, Fs&&... tail) {
  head.GetCore()->SetCallbackInline(combinator);
  std::move(head).Detach();
  if constexpr (sizeof...(tail) != 0) {
    WhenAnyImpl(combinator, std::forward<Fs>(tail)...);
  }
}

template <PolicyWhenAny P = PolicyWhenAny::FirstError, typename It,
          typename T = util::detail::FutureValueT<typename std::iterator_traits<It>::value_type>, typename Indx>
auto WhenAnyImpl(It iter, Indx begin, Indx end) {
  auto [future, combinator] = [&] {
    if constexpr (std::is_same_v<It, Indx>) {
      if constexpr (P == PolicyWhenAny::FirstError) {
        return detail::AnyCombinator<T, P>::Make(begin != end);
      } else {
        return detail::AnyCombinator<T, P>::Make(std::distance(begin, end));
      }
    } else {
      return detail::AnyCombinator<T, P>::Make(end);
    }
  }();
  for (; begin != end; ++begin) {
    iter->GetCore()->SetCallbackInline(combinator);
    std::move(*iter).Detach();
    ++iter;
  }
  return std::move(future);
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
  return detail::WhenAnyImpl(begin, size_t{0}, size);
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
  return detail::WhenAnyImpl(begin, begin, end);
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
auto WhenAny(Future<T>&& head, Fs&&... tail) {
  static_assert((... && util::IsFutureV<Fs>));
  auto [future, combinator] = detail::AnyCombinator<T, P>::Make(sizeof...(Fs) + 1);
  detail::WhenAnyImpl<P>(combinator, std::move(head), std::forward<Fs>(tail)...);
  return std::move(future);
}

}  // namespace yaclib

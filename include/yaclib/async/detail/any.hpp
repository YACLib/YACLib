#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

#include <atomic>

namespace yaclib::detail {

template <typename... Cores>
struct AnyBase {
  static constexpr ConsumePolicy ConsumeP = ConsumePolicy::Unordered;
  static constexpr CorePolicy CoreP = CorePolicy::Managed;

  using ValueType = typename MaybeVariant<typename Unique<std::tuple<typename Cores::Value...>>::Type>::Type;
  using ErrorType = head_t<typename Cores::Error...>;
  static_assert((... && std::is_same_v<ErrorType, typename Cores::Error>));

  explicit AnyBase(Promise<ValueType, ErrorType> p) : _p{std::move(p)} {
  }

 protected:
  Promise<ValueType, ErrorType> _p;
};

template <FailPolicy P, typename... Cores>
struct Any;

template <typename... Cores>
struct Any<FailPolicy::None, Cores...> : public AnyBase<Cores...> {
  using Base = AnyBase<Cores...>;

  using typename Base::ErrorType;
  using typename Base::ValueType;

  Any(size_t count, Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (!_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_relaxed)) {
      std::move(this->_p).Set(std::forward<Result>(result));
    }
  }

  yaclib_std::atomic_bool _done = false;
};

template <typename... Cores>
struct Any<FailPolicy::FirstFail, Cores...> : public AnyBase<Cores...> {
  using Base = AnyBase<Cores...>;

  using typename Base::ErrorType;
  using typename Base::ValueType;

  Any(size_t count, Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (result) {
      auto old = _state.exchange(State::kValue, std::memory_order_relaxed);
      if (old != State::kValue) {
        std::move(this->_p).Set(std::forward<Result>(result).Value());
      }
    } else {
      State expected = State::kEmpty;
      if (_state.compare_exchange_strong(expected, State::kError, std::memory_order_relaxed)) {
        if (result.State() == ResultState::Error) {
          error = std::forward<Result>(result).Error();
        } else {
          error = std::forward<Result>(result).Exception();
        }
      }
    }
  }

  ~Any() {
    if (_state.load(std::memory_order_relaxed) != State::kValue) {
      std::move(this->_p).Set(std::move(error));
    }
  }

 private:
  enum class State {
    kEmpty,
    kError,
    kValue,
  };

  yaclib_std::atomic<State> _state = State::kEmpty;
  Result<ValueType, ErrorType> error;
};

template <typename... Cores>
struct Any<FailPolicy::LastFail, Cores...> : public AnyBase<Cores...> {
  using Base = AnyBase<Cores...>;

  using typename Base::ErrorType;
  using typename Base::ValueType;

  Any(size_t count, Promise<ValueType, ErrorType> p) : Base{std::move(p)}, _state{2 * count} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (!DoneImpl(_state.load(std::memory_order_relaxed))) {
      if (result) {
        if (!DoneImpl(_state.exchange(1, std::memory_order_relaxed))) {
          std::move(this->_p).Set(std::forward<Result>(result));
        }
      } else if (_state.fetch_sub(2, std::memory_order_relaxed) == 2) {
        std::move(this->_p).Set(std::forward<Result>(result));
      }
    }
  }

 private:
  static bool DoneImpl(std::size_t value) noexcept {
    return (value & 1U) != 0;
  }

  yaclib_std::atomic_size_t _state;
};

}  // namespace yaclib::detail

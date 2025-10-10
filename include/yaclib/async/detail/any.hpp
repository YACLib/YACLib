#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

#include <atomic>

namespace yaclib::detail {

template <FailPolicy P, typename OutputValue, typename OutputError, typename InputCore>
struct Any;

template <typename T>
struct IsVariant : std::false_type {};

template <typename... Ts>
struct IsVariant<std::variant<Ts...>> : std::true_type {};

template <typename OutputValue, typename OutputError, typename InputCore>
struct Any<FailPolicy::None, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (!_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_acq_rel)) {
      if constexpr (IsVariant<OutputValue>::value) {
        std::move(this->_p).Set(OutputValue{std::forward<Result>(result).Value()});
      } else {
        std::move(this->_p).Set(std::forward<Result>(result).Value());
      }
    }
  }

  yaclib_std::atomic_bool _done = false;
  PromiseType _p;
};

template <typename OutputValue, typename OutputError, typename InputCore>
struct Any<FailPolicy::FirstFail, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (result) {
      if (_state.load(std::memory_order_relaxed) != State::kValue &&
          _state.exchange(State::kValue, std::memory_order_acq_rel) != State::kValue) {
        if constexpr (IsVariant<OutputValue>::value) {
          std::move(this->_p).Set(OutputValue{std::forward<Result>(result).Value()});
        } else {
          std::move(this->_p).Set(std::forward<Result>(result).Value());
        }
      }
    } else {
      State expected = State::kEmpty;
      if (_state.load(std::memory_order_relaxed) == expected &&
          _state.compare_exchange_strong(expected, State::kError, std::memory_order_acq_rel)) {
        if (result.State() == ResultState::Error) {
          error = std::forward<Result>(result).Error();
        } else {
          error = std::forward<Result>(result).Exception();
        }
      }
    }
  }

  ~Any() {
    if (_state.load(std::memory_order_acquire) != State::kValue) {
      if (error.State() == ResultState::Error) {
        std::move(this->_p).Set(std::move(error).Error());
      } else {
        std::move(this->_p).Set(std::move(error).Exception());
      }
    }
  }

 private:
  enum class State {
    kEmpty,
    kError,
    kValue,
  };

  yaclib_std::atomic<State> _state = State::kEmpty;
  Result<void, OutputError> error;
  PromiseType _p;
};

template <typename OutputValue, typename OutputError, typename InputCore>
struct Any<FailPolicy::LastFail, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _state{2 * count}, _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (!DoneImpl(_state.load(std::memory_order_acquire))) {
      if (result) {
        if (!DoneImpl(_state.exchange(1, std::memory_order_acq_rel))) {
          if constexpr (IsVariant<OutputValue>::value) {
            std::move(this->_p).Set(OutputValue{std::forward<Result>(result).Value()});
          } else {
            std::move(this->_p).Set(std::forward<Result>(result));
          }
        }
      } else if (_state.fetch_sub(2, std::memory_order_acq_rel) == 2) {
        if (result.State() == ResultState::Error) {
          std::move(this->_p).Set(std::forward<Result>(result).Error());
        } else {
          std::move(this->_p).Set(std::forward<Result>(result).Exception());
        }
      }
    }
  }

 private:
  static bool DoneImpl(std::size_t value) noexcept {
    return (value & 1U) != 0;
  }

  yaclib_std::atomic_size_t _state;
  PromiseType _p;
};

}  // namespace yaclib::detail

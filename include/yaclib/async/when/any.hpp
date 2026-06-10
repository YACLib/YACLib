#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

#include <atomic>
#include <optional>

namespace yaclib::when {

template <FailPolicy F, typename OutputValue, typename Trait, typename InputCore>
struct Any;

template <typename OutputValue, typename Trait, typename InputCore>
struct Any<FailPolicy::None, OutputValue, Trait, InputCore> {
  using PromiseType = Promise<OutputValue, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename R>
  void Consume(R&& result) {
    if (!_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_acq_rel)) {
      if (Trait::Ok(result)) {
        std::move(_p).Set(Trait::GetValue(std::forward<R>(result)));
      } else {
        std::move(_p).Set(Trait::GetError(std::forward<R>(result)));
      }
    }
  }

  yaclib_std::atomic_bool _done = false;
  PromiseType _p;
};

template <typename OutputValue, typename Trait, typename InputCore>
struct Any<FailPolicy::FirstFail, OutputValue, Trait, InputCore> {
  using PromiseType = Promise<OutputValue, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename R>
  void Consume(R&& result) {
    if (Trait::Ok(result)) {
      if (_state.load(std::memory_order_relaxed) != State::kValue &&
          _state.exchange(State::kValue, std::memory_order_acq_rel) != State::kValue) {
        std::move(_p).Set(Trait::GetValue(std::forward<R>(result)));
      }
    } else {
      State expected = State::kEmpty;
      if (_state.load(std::memory_order_relaxed) == expected &&
          _state.compare_exchange_strong(expected, State::kError, std::memory_order_acq_rel)) {
        _error.emplace(Trait::GetError(std::forward<R>(result)));
      }
    }
  }

  ~Any() {
    if (_p.Valid()) {
      YACLIB_ASSERT(_error.has_value());
      std::move(_p).Set(std::move(*_error));
    }
  }

 private:
  enum class State {
    kEmpty,
    kError,
    kValue,
  };

  yaclib_std::atomic<State> _state = State::kEmpty;
  std::optional<typename Trait::Error> _error;
  PromiseType _p;
};

template <typename OutputValue, typename Trait, typename InputCore>
struct Any<FailPolicy::LastFail, OutputValue, Trait, InputCore> {
  using PromiseType = Promise<OutputValue, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _state{2 * count}, _p{std::move(p)} {
  }

  template <typename R>
  void Consume(R&& result) {
    if (!DoneImpl(_state.load(std::memory_order_acquire))) {
      if (Trait::Ok(result)) {
        if (!DoneImpl(_state.exchange(1, std::memory_order_acq_rel))) {
          std::move(_p).Set(Trait::GetValue(std::forward<R>(result)));
        }
      } else if (_state.fetch_sub(2, std::memory_order_acq_rel) == 2) {
        std::move(_p).Set(Trait::GetError(std::forward<R>(result)));
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

}  // namespace yaclib::when

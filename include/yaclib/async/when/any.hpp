#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

#include <atomic>

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
  using Error = typename Trait::Error;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Any(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <typename R>
  void Consume(R&& result) {
    if (Trait::Ok(result)) {
      if ((_state.load(std::memory_order_relaxed) & kValue) == 0 &&
          (_state.fetch_or(kValue, std::memory_order_acq_rel) & kValue) == 0) {
        std::move(_p).Set(Trait::GetValue(std::forward<R>(result)));
      }
    } else {
      // kError is an exclusive reservation taken before constructing _error,
      // only the destructor reads it afterwards, ordered by the combinator refcount
      if (_state.load(std::memory_order_relaxed) == kEmpty &&
          (_state.fetch_or(kError, std::memory_order_acq_rel) & kError) == 0) {
        ::new (&_error.error) Error{Trait::GetError(std::forward<R>(result))};
      }
    }
  }

  ~Any() {
    const auto state = _state.load(std::memory_order_relaxed);
    if (_p.Valid()) {
      YACLIB_ASSERT((state & kError) != 0);
      std::move(_p).Set(std::move(_error.error));
    }
    // A stored error is destroyed even when a value won and the promise was set by Consume
    if ((state & kError) != 0) {
      _error.error.~Error();
    }
  }

 private:
  static constexpr unsigned char kEmpty = 0;
  static constexpr unsigned char kValue = 1;
  static constexpr unsigned char kError = 2;

  union State {
    YACLIB_NO_UNIQUE_ADDRESS Unit stub;
    YACLIB_NO_UNIQUE_ADDRESS Error error;

    State() noexcept : stub{} {
    }
    ~State() noexcept {
    }
  };

  yaclib_std::atomic<unsigned char> _state = kEmpty;
  YACLIB_NO_UNIQUE_ADDRESS State _error;
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

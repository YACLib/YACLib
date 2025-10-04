#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>

#include <atomic>

namespace yaclib::detail {

template <FailPolicy F, typename Error>
struct Join;

template <>
struct Join<FailPolicy::None, StopError> {
  static constexpr ConsumePolicy ConsumeP = ConsumePolicy::None;
  static constexpr CorePolicy CoreP = CorePolicy::Managed;

  using ValueType = void;
  using ErrorType = StopError;
  using PromiseType = Promise<ValueType, StopError>;

  Join(std::size_t count, PromiseType p) noexcept : _p{std::move(p)} {
  }

  ~Join() {
    std::move(_p).Set();
  }

 private:
  PromiseType _p;
};

template <typename Error>
struct Join<FailPolicy::FirstFail, Error> {
  static constexpr ConsumePolicy ConsumeP = ConsumePolicy::Unordered;
  static constexpr CorePolicy CoreP = CorePolicy::Managed;

  using ValueType = void;
  using ErrorType = Error;
  using PromiseType = Promise<ValueType, StopError>;

  Join(std::size_t count, PromiseType p) noexcept : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (!result && !_done.exchange(true, std::memory_order_relaxed)) {
      if (result.State() == ResultState::Error) {
        std::move(_p).Set(std::forward<Result>(result).Error());
      } else {
        std::move(_p).Set(std::forward<Result>(result).Exception());
      }
    }
  }

  ~Join() {
    if (!_done.load(std::memory_order_relaxed)) {
      std::move(_p).Set();
    }
  }

 private:
  yaclib_std::atomic_bool _done;
  PromiseType _p;
};

}  // namespace yaclib::detail

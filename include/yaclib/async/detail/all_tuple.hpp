#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

template <FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct AllTuple;

template <typename OutputValue, typename OutputError, typename InputCore>
struct AllTuple<FailPolicy::None, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Static;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  AllTuple(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <std::size_t Index, typename Result>
  void Consume(Result&& result) {
    std::get<Index>(_tuple) = std::forward<Result>(result);
  }

  ~AllTuple() {
    std::move(this->_p).Set(std::move(_tuple));
  }

 private:
  OutputValue _tuple;
  PromiseType _p;
};

template <typename OutputValue, typename OutputError, typename InputCore>
struct AllTuple<FailPolicy::FirstFail, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Static;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  AllTuple(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <std::size_t Index, typename Result>
  void Consume(Result&& result) {
    if (!result && !_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_acq_rel)) {
      if (result.State() == ResultState::Error) {
        std::move(this->_p).Set(std::forward<Result>(result).Error());
      } else {
        std::move(this->_p).Set(std::forward<Result>(result).Exception());
      }
    } else {
      std::get<Index>(_tuple) = std::forward<Result>(result).Value();
    }
  }

  ~AllTuple() {
    if (!_done.load(std::memory_order_acquire)) {
      std::move(this->_p).Set(std::move(_tuple));
    }
  }

 private:
  yaclib_std::atomic_bool _done = false;
  OutputValue _tuple;
  PromiseType _p;
};

}  // namespace yaclib::detail

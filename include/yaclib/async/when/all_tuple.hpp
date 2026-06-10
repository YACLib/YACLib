#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::when {

template <FailPolicy F, typename OutputValue, typename Trait, typename InputCore>
struct AllTuple {
  static_assert(F != FailPolicy::LastFail, "LastFail policy is not supported by AllTuple");
};

template <typename OutputValue, typename Trait, typename InputCore>
struct AllTuple<FailPolicy::None, OutputValue, Trait, InputCore> {
  using PromiseType = Promise<OutputValue, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Static;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  AllTuple(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <std::size_t Index, typename R>
  void Consume(R&& result) {
    std::get<Index>(_tuple) = std::forward<R>(result);
  }

  ~AllTuple() {
    std::move(_p).Set(std::move(_tuple));
  }

 private:
  OutputValue _tuple;
  PromiseType _p;
};

template <typename OutputValue, typename Trait, typename InputCore>
struct AllTuple<FailPolicy::FirstFail, OutputValue, Trait, InputCore> {
  using PromiseType = Promise<OutputValue, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Static;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  AllTuple(std::size_t count, PromiseType p) : _p{std::move(p)} {
  }

  template <std::size_t Index, typename R>
  void Consume(R&& result) {
    if (Trait::Ok(result)) {
      std::get<Index>(_tuple) = Trait::GetValue(std::forward<R>(result));
    } else if (!_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_acq_rel)) {
      std::move(_p).Set(Trait::GetError(std::forward<R>(result)));
    }
    // An error that lost the _done race is dropped, the promise is already set
  }

  ~AllTuple() {
    if (_p.Valid()) {
      std::move(_p).Set(std::move(_tuple));
    }
  }

 private:
  yaclib_std::atomic_bool _done = false;
  OutputValue _tuple;
  PromiseType _p;
};

}  // namespace yaclib::when

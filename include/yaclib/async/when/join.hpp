#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>

#include <atomic>

namespace yaclib::when {

template <FailPolicy F, typename OutputValue, typename Trait, typename InputCore>
struct Join {
  static_assert(F != FailPolicy::LastFail, "LastFail policy is not supported by Join");
  static_assert(std::is_void_v<OutputValue>, "OutputValue should be void for Join");
};

template <typename Trait, typename InputCore>
struct Join<FailPolicy::None, void, Trait, InputCore> {
  using PromiseType = Promise<void, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::None;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Join(std::size_t count, PromiseType p) noexcept : _p{std::move(p)} {
  }

  ~Join() {
    std::move(_p).Set();
  }

 private:
  PromiseType _p;
};

template <typename Trait, typename InputCore>
struct Join<FailPolicy::FirstFail, void, Trait, InputCore> {
  using PromiseType = Promise<void, Trait>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Join(std::size_t count, PromiseType p) noexcept : _p{std::move(p)} {
  }

  template <typename R>
  void Consume(R&& result) {
    if (!Trait::Ok(result) && !_done.load(std::memory_order_relaxed) &&
        !_done.exchange(true, std::memory_order_acq_rel)) {
      std::move(_p).Set(Trait::GetError(std::forward<R>(result)));
    }
  }

  ~Join() {
    if (_p.Valid()) {
      std::move(_p).Set();
    }
  }

 private:
  yaclib_std::atomic_bool _done = false;
  PromiseType _p;
};

}  // namespace yaclib::when

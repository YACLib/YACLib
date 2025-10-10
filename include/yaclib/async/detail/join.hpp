#pragma once

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>

#include <atomic>

namespace yaclib::detail {

template <FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct Join {
  static_assert(F != FailPolicy::LastFail, "LastFail policy is not supported by Join");
  static_assert(std::is_void_v<OutputValue>);
};

template <typename OutputError, typename InputCore>
struct Join<FailPolicy::None, void, OutputError, InputCore> {
  using PromiseType = Promise<void, OutputError>;

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

template <typename OutputError, typename InputCore>
struct Join<FailPolicy::FirstFail, void, OutputError, InputCore> {
  using PromiseType = Promise<void, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Managed;

  Join(std::size_t count, PromiseType p) noexcept : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (!result && !_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_acq_rel)) {
      if (result.State() == ResultState::Error) {
        std::move(this->_p).Set(std::forward<Result>(result).Error());
      } else {
        std::move(this->_p).Set(std::forward<Result>(result).Exception());
      }
    }
  }

  ~Join() {
    if (!_done.load(std::memory_order_acquire)) {
      std::move(this->_p).Set();
    }
  }

 private:
  yaclib_std::atomic_bool _done = false;
  PromiseType _p;
};

}  // namespace yaclib::detail

#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

#include <vector>

namespace yaclib::detail {

template <FailPolicy F, typename OutputValue, typename OutputError, typename InputCore>
struct All;

template <typename OutputValue, typename OutputError, typename InputCore>
struct All<FailPolicy::None, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::None;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Owned;

  All(size_t count, PromiseType p) : _p{std::move(p)} {
    _cores.resize(count);
  }

  void Register(size_t i, InputCore& core) {
    _cores[i] = &core;
  }

  ~All() {
    OutputValue output;
    output.reserve(_cores.size());
    for (auto* core : _cores) {
      auto core_result = core->Retire();
      if (core_result) {
        output.push_back(std::move(core_result).Value());
      }
    }
    std::move(this->_p).Set(std::move(output));
  }

 private:
  std::vector<InputCore*> _cores;
  PromiseType _p;
};

template <typename OutputValue, typename OutputError, typename InputCore>
struct All<FailPolicy::FirstFail, OutputValue, OutputError, InputCore> {
  using PromiseType = Promise<OutputValue, OutputError>;

  static constexpr ConsumePolicy kConsumePolicy = ConsumePolicy::Unordered;
  static constexpr CorePolicy kCorePolicy = CorePolicy::Owned;

  All(size_t count, PromiseType p) : _p{std::move(p)} {
    _cores.resize(count);
  }

  void Register(size_t i, InputCore& core) {
    _cores[i] = &core;
  }

  void Consume(InputCore& core) {
    auto& result = core.Get();
    if (!result && !_done.load(std::memory_order_relaxed) && !_done.exchange(true, std::memory_order_acq_rel)) {
      if (result.State() == ResultState::Exception) {
        std::move(this->_p).Set(std::as_const(result).Exception());
      } else {
        std::move(this->_p).Set(std::as_const(result).Error());
      }
    }
  }

  ~All() {
    if (_done.load(std::memory_order_acquire)) {
      for (auto* core : _cores) {
        core->DecRef();
      }
    } else {
      OutputValue result;
      result.reserve(_cores.size());
      for (auto* core : _cores) {
        result.push_back(core->Retire().Value());
      }
      std::move(this->_p).Set(std::move(result));
    }
  }

 private:
  std::vector<InputCore*> _cores;
  yaclib_std::atomic_bool _done = false;
  PromiseType _p;
};

}  // namespace yaclib::detail

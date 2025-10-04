#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

#include <vector>

namespace yaclib::detail {

enum class ElementType {
  Result,
  Value,
};

// Core might be:
// ResultCore<V, E> : in case of mixed unique and shared cores
// UniqueCore<V, E> : in case of unique cores only
// SharedCore<V, E> : in case of shared cores only
// Passing UniqueCore/SharedCore devirtualizes the call to Retire
template <FailPolicy F, ElementType E, template <typename...> typename Container, typename Core>
struct AllBase {
  using ElementValue = typename Core::Value;
  using ElementError = typename Core::Error;
  using ReturnElement =
    std::conditional_t<E == ElementType::Result, Result<ElementValue, ElementError>, wrap_void_t<ElementValue>>;

  using ValueType = Container<ReturnElement>;
  using ErrorType = std::conditional_t<F == FailPolicy::None, StopError, ElementError>;

  AllBase(Promise<ValueType, ErrorType> p) noexcept : _p{std::move(p)} {
  }

 protected:
  Promise<ValueType, ErrorType> _p;
};

template <FailPolicy F, ElementType E, template <typename...> typename Container, typename ResultCore>
struct All;

template <ElementType E, template <typename...> typename Container, typename Core>
struct All<FailPolicy::None, E, Container, Core> : public AllBase<FailPolicy::None, E, Container, Core> {
  using Base = AllBase<FailPolicy::None, E, Container, Core>;
  using typename Base::ErrorType;
  using typename Base::ValueType;

  static constexpr ConsumePolicy ConsumeP = ConsumePolicy::None;
  static constexpr CorePolicy CoreP = CorePolicy::Owned;

  All(size_t count, yaclib::Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
    _cores.resize(count);
  }

  void Register(size_t i, Core& core) {
    _cores[i] = &core;
  }

  ~All() {
    ValueType result;
    result.reserve(_cores.size());
    for (auto* core : _cores) {
      if constexpr (E == ElementType::Result) {
        result.push_back(core->Retire());
      } else {
        auto core_result = core->Retire();
        if (core_result) {
          result.push_back(std::move(core_result).Value());
        }
      }
    }
    std::move(this->_p).Set(std::move(result));
  }

 private:
  std::vector<Core*> _cores;
};

// ElementType E is ignored since it makes no sense to return successful results only
template <ElementType E, template <typename...> typename Container, typename Core>
struct All<FailPolicy::FirstFail, E, Container, Core> : public AllBase<FailPolicy::FirstFail, E, Container, Core> {
  using Base = AllBase<FailPolicy::FirstFail, E, Container, Core>;
  using typename Base::ErrorType;
  using typename Base::ValueType;

  static constexpr ConsumePolicy ConsumeP = ConsumePolicy::Unordered;
  static constexpr CorePolicy CoreP = CorePolicy::Owned;

  All(size_t count, yaclib::Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
    _cores.resize(count);
  }

  void Register(size_t i, Core& core) {
    _cores[i] = &core;
  }

  void Consume(Core& core) {
    auto& result = core.Get();
    if (!result && !_done.exchange(true, std::memory_order_relaxed)) {
      if (result.State() == ResultState::Exception) {
        std::move(this->_p).Set(std::as_const(result).Exception());
      } else {
        std::move(this->_p).Set(std::as_const(result).Error());
      }
    }
  }

  ~All() {
    if (_done.load(std::memory_order_relaxed)) {
      for (auto* core : _cores) {
        core->DecRef();
      }
    } else {
      ValueType result;
      result.reserve(_cores.size());
      for (auto* core : _cores) {
        result.push_back(core->Retire().Value());
      }
      std::move(this->_p).Set(std::move(result));
    }
  }

 private:
  std::vector<Core*> _cores;
  yaclib_std::atomic_bool _done = false;
};

}  // namespace yaclib::detail

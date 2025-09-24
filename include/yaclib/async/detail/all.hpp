#pragma once

#include <yaclib_std/detail/atomic.hpp>

#include <yaclib/async/promise.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/order_policy.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

#include <vector>

namespace yaclib::detail {

struct Join {
  static constexpr StrategyOrder Order = StrategyOrder::None;
  static constexpr StrategyPolicy Policy = StrategyPolicy::Managed;

  using ValueType = void;
  using ErrorType = StopError;
  using PromiseType = Promise<ValueType, StopError>;

  Join(std::size_t count, PromiseType p) noexcept : _p{std::move(p)} {
  }

  template <typename Result>
  void Consume(Result&& result) {
  }

  ~Join() {
    std::move(_p).Set();
  }

 private:
  PromiseType _p;
};

template <FailPolicy F, OrderPolicy O, typename ResultCore>
struct AllVectorBase {
  using ElementValue = typename ResultCore::Value;
  using ElementError = typename ResultCore::Error;
  using ReturnElement = std::conditional_t<F == FailPolicy::None, Result<ElementValue, ElementError>,
                                           wrap_bool_t<wrap_void_t<ElementValue>>>;

  using ValueType = std::vector<ReturnElement>;
  using ErrorType = std::conditional_t<F == FailPolicy::None, StopError, ElementError>;

  AllVectorBase(Promise<ValueType, ErrorType> p) noexcept : _p{std::move(p)} {
  }

 protected:
  Promise<ValueType, ErrorType> _p;
};

template <FailPolicy F, OrderPolicy O, typename ResultCore>
struct AllVector;

template <typename ResultCore>
struct AllVector<FailPolicy::None, OrderPolicy::Fifo, ResultCore>
  : public AllVectorBase<FailPolicy::None, OrderPolicy::Fifo, ResultCore> {
  using Base = AllVectorBase<FailPolicy::None, OrderPolicy::Fifo, ResultCore>;
  using typename Base::ErrorType;
  using typename Base::ReturnElement;
  using typename Base::ValueType;

  static constexpr StrategyOrder Order = StrategyOrder::None;
  static constexpr StrategyPolicy Policy = StrategyPolicy::Managed;

  AllVector(size_t count, yaclib::Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
    _vector.resize(count);
  }

  template <typename Result>
  void Consume(Result&& result) {
    auto ticket = _ticket.fetch_add(1);
    this->_vector[ticket].~ReturnElement();
    new (&this->_vector[ticket]) ReturnElement{std::forward<Result>(result)};
  }

  ~AllVector() {
    std::move(this->_p).Set(std::move(_vector));
  }

 private:
  ValueType _vector;
  yaclib_std::atomic_size_t _ticket = 0;
};

template <typename ResultCore>
struct AllVector<FailPolicy::FirstFail, OrderPolicy::Fifo, ResultCore>
  : public AllVectorBase<FailPolicy::FirstFail, OrderPolicy::Fifo, ResultCore> {
  using Base = AllVectorBase<FailPolicy::FirstFail, OrderPolicy::Fifo, ResultCore>;
  using typename Base::ErrorType;
  using typename Base::ValueType;

  static constexpr StrategyOrder Order = StrategyOrder::None;
  static constexpr StrategyPolicy Policy = StrategyPolicy::Managed;

  AllVector(size_t count, yaclib::Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
    _vector.resize(count);
  }

  template <typename Result>
  void Consume(Result&& result) {
    if (result) {
      auto ticket = _ticket.fetch_add(1);
      if (!_done.load()) {
        _vector[ticket] = std::forward<Result>(result).Value();
      }
    } else {
      if (!_done.exchange(true)) {
        if (result.State() == ResultState::Error) {
          std::move(this->_p).Set(std::forward<Result>(result).Error());
        } else {
          std::move(this->_p).Set(std::forward<Result>(result).Exception());
        }
      }
    }
  }

  ~AllVector() {
    if (!_done.load()) {
      std::move(this->_p).Set(std::move(_vector));
    }
  }

 private:
  ValueType _vector;
  yaclib_std::atomic_size_t _ticket = 0;
  yaclib_std::atomic_bool _done = false;
};

template <typename ResultCore>
struct AllVector<FailPolicy::None, OrderPolicy::Same, ResultCore>
  : public AllVectorBase<FailPolicy::None, OrderPolicy::Same, ResultCore> {
  using Base = AllVectorBase<FailPolicy::None, OrderPolicy::Same, ResultCore>;
  using typename Base::ErrorType;
  using typename Base::ValueType;

  static constexpr StrategyOrder Order = StrategyOrder::None;
  static constexpr StrategyPolicy Policy = StrategyPolicy::Owned;

  AllVector(size_t count, yaclib::Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
    _cores.resize(count);
  }

  void Register(size_t i, ResultCore& core) {
    _cores[i] = &core;
  }

  template <typename Core>
  void Consume(Core& core) {
  }

  ~AllVector() {
    ValueType result;
    result.reserve(_cores.size());
    for (auto* core : _cores) {
      result.push_back(core->Retire());
    }
    std::move(this->_p).Set(std::move(result));
  }

 private:
  std::vector<ResultCore*> _cores;
};

template <typename ResultCore>
struct AllVector<FailPolicy::FirstFail, OrderPolicy::Same, ResultCore>
  : public AllVectorBase<FailPolicy::FirstFail, OrderPolicy::Same, ResultCore> {
  using Base = AllVectorBase<FailPolicy::FirstFail, OrderPolicy::Same, ResultCore>;
  using typename Base::ErrorType;
  using typename Base::ValueType;

  static constexpr StrategyOrder Order = StrategyOrder::None;
  static constexpr StrategyPolicy Policy = StrategyPolicy::Owned;

  AllVector(size_t count, yaclib::Promise<ValueType, ErrorType> p) : Base{std::move(p)} {
    _cores.resize(count);
  }

  void Register(size_t i, ResultCore& core) {
    _cores[i] = &core;
  }

  template <typename Core>
  void Consume(Core& core) {
    auto& result = core.Get();
    if (!result && !_done.exchange(true)) {
      if (result.State() == ResultState::Exception) {
        std::move(this->_p).Set(std::as_const(result).Exception());
      } else {
        std::move(this->_p).Set(std::as_const(result).Error());
      }
    }
  }

  ~AllVector() {
    if (_done.load()) {
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
  std::vector<ResultCore*> _cores;
  yaclib_std::atomic_bool _done = false;
};

}  // namespace yaclib::detail

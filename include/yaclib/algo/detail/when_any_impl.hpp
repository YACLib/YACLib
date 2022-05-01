#pragma once

#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <cstddef>
#include <exception>
#include <utility>
#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename V, typename E, WhenPolicy P /*None*/>
class AnyCombinatorBase {
 protected:
  yaclib_std::atomic_bool _done;
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t /*count*/, ResultCorePtr<V, E>&& core) : _done{false}, _core{std::move(core)} {
  }

  bool Done() {
    return _done.load(std::memory_order_acquire);
  }

  void Combine(Result<V, E>&& result) {
    if (!_done.exchange(true, std::memory_order_acq_rel)) {
      _core->Set(std::move(result));
    }
  }
};

template <typename V, typename E>
class AnyCombinatorBase<V, E, WhenPolicy::LastFail> {
  yaclib_std::atomic_size_t _state;

 protected:
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t count, ResultCorePtr<V, E>&& core)
      : _state{2 * count}, _core{std::move(core)} {
  }

  bool Done() {
    return (_state.load(std::memory_order_acquire) & 1U) != 0;
  }

  void Combine(Result<V, E>&& result) {
    if (result) {
      if ((_state.exchange(1, std::memory_order_acq_rel) & 1U) == 0) {
        _core->Set(std::move(result));
      }
    } else if (_state.fetch_sub(2, std::memory_order_acq_rel) == 2) {
      _core->Set(std::move(result));
    }
  }
};

template <typename V, typename E>
class AnyCombinatorBase<V, E, WhenPolicy::FirstFail> : public AnyCombinatorBase<V, E, WhenPolicy::None> {
  using Base = AnyCombinatorBase<V, E, WhenPolicy::None>;

  yaclib_std::atomic<ResultState> _state;
  union {
    Unit _unit;
    E _error;
    std::exception_ptr _exception;
  };

 protected:
  explicit AnyCombinatorBase(std::size_t count, ResultCorePtr<V, E>&& core)
      : Base{count, std::move(core)}, _state{ResultState::Empty}, _unit{} {
  }

  ~AnyCombinatorBase() {
    auto done = this->_done.load(std::memory_order_acquire);
    auto state = _state.load(std::memory_order_acquire);
    if (state == ResultState::Error) {
      if (!done) {
        this->_core->Set(std::move(_error));
      }
      _error.~E();
    } else if (state == ResultState::Exception) {
      if (!done) {
        this->_core->Set(std::move(_exception));
      }
      _exception.~exception_ptr();
    }
  }

  void Combine(Result<V, E>&& result) {
    auto state = result.State();
    if (state == ResultState::Value) {
      return Base::Combine(std::move(result));
    }
    auto old_state = _state.load(std::memory_order_acquire);
    if (old_state == ResultState::Empty &&
        _state.compare_exchange_strong(old_state, state, std::memory_order_acq_rel, std::memory_order_relaxed)) {
      if (state == ResultState::Exception) {
        new (&_exception) std::exception_ptr{std::move(result).Exception()};
      } else {
        YACLIB_DEBUG(state != ResultState::Error, "state should be Error, but here it's Empty");
        new (&_error) E{std::move(result).Error()};
      }
    }
  }
};

template <typename V, typename E, WhenPolicy P>
class AnyCombinator : public AnyCombinatorBase<V, E, P>, public InlineCore {
  using Base = AnyCombinatorBase<V, E, P>;
  using Base::Base;

 public:
  static auto Make(std::size_t count) {
    // TODO(MBkkt) Maybe single allocation instead of two?
    auto raw_core = new AtomicCounter<ResultCore<V, E>>{2};
    ResultCorePtr<V, E> combine_core{NoRefTag{}, raw_core};
    ResultCorePtr<V, E> future_core{NoRefTag{}, raw_core};
    auto combinator = new AtomicCounter<AnyCombinator<V, E, P>>{count, count, std::move(combine_core)};
    return std::pair{std::move(future_core), combinator};
  }

 private:
  void CallInline(InlineCore& caller, State) noexcept final {
    if (this->_core->Alive() && !this->Done()) {
      auto& core = static_cast<ResultCore<V, E>&>(caller);
      this->Combine(std::move(core.Get()));
    }
  }
};

}  // namespace yaclib::detail

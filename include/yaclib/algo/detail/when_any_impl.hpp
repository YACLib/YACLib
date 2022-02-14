#pragma once

#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/fault/atomic.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename V, typename E, WhenPolicy P /*None*/>
class AnyCombinatorBase {
 protected:
  yaclib_std::atomic_bool _done;
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t /*size*/, ResultCorePtr<V, E>&& core) : _done{false}, _core{std::move(core)} {
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

  explicit AnyCombinatorBase(std::size_t size, ResultCorePtr<V, E>&& core) : _state{2 * size}, _core{std::move(core)} {
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
  explicit AnyCombinatorBase(std::size_t size, ResultCorePtr<V, E>&& core)
      : Base{size, std::move(core)}, _state{ResultState::Empty}, _unit{} {
  }

  ~AnyCombinatorBase() {
    auto done = Base::_done.load(std::memory_order_acquire);
    auto state = _state.load(std::memory_order_acquire);
    if (state == ResultState::Error) {
      if (!done) {
        Base::_core->Set(std::move(_error));
      }
      _error.~E();
    } else if (state == ResultState::Exception) {
      if (!done) {
        Base::_core->Set(std::move(_exception));
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
        assert(state == ResultState::Error);
        new (&_error) E{std::move(result).Error()};
      }
    }
  }
};

template <typename V, typename E, WhenPolicy P>
class AnyCombinator : public AnyCombinatorBase<V, E, P>, public InlineCore {
  using Base = AnyCombinatorBase<V, E, P>;

  void CallInline(InlineCore* context, State) noexcept final {
    if (Base::_core->Alive() && !Base::Done()) {
      Base::Combine(std::move(static_cast<ResultCore<V, E>*>(context)->Get()));
    }
  }

 public:
  static auto Make(std::size_t size) {
    assert(size >= 2);
    // TODO(MBkkt) Should be single allocation
    auto core = MakeIntrusive<detail::ResultCore<V, E>>();
    auto combinator = MakeIntrusive<AnyCombinator<V, E, P>>(size, core);
    return std::pair{Future<V, E>{std::move(core)}, std::move(combinator)};
  }

  explicit AnyCombinator(std::size_t size, ResultCorePtr<V, E> core) : Base{size, std::move(core)} {
  }
};

template <typename V, typename E, WhenPolicy P>
using AnyCombinatorPtr = IntrusivePtr<AnyCombinator<V, E, P>>;

template <WhenPolicy P, typename V, typename E, typename ValueIt, typename RangeIt>
Future<V, E> WhenAnyImpl(ValueIt it, RangeIt begin, RangeIt end) {
  if (begin == end) {
    return {};
  }
  if (auto next = begin; ++next == end) {
    return std::move(*it);
  }
  const auto combinator_size = [&]() -> std::size_t {
    if constexpr (P == WhenPolicy::None || P == WhenPolicy::FirstFail) {
      return 2;
    } else {
      // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
      // Maybe the user has the size of the range, or he is satisfied with another WhenPolicy,
      // as a last resort it is suggested to call WhenAny<WhenPolicy::LastFail>(begin, distance(begin, end))
      return end - begin;
    }
  }();
  auto [future, combinator] = AnyCombinator<V, E, P>::Make(combinator_size);
  for (; begin != end; ++begin) {
    std::exchange(it->GetCore(), nullptr)->SetCallbackInline(combinator);
    ++it;
  }
  return std::move(future);
}

template <WhenPolicy P, typename V, typename E, typename... Vs, typename... Es>
void WhenAnyImpl(AnyCombinatorPtr<V, E, P>& combinator, Future<Vs, Es>&&... futures) {
  (..., std::exchange(futures.GetCore(), nullptr)->SetCallbackInline(combinator));
}

}  // namespace yaclib::detail

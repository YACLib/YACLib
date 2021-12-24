#pragma once

#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename T, WhenPolicy P /*None*/>
class AnyCombinatorBase {
 protected:
  yaclib_std::atomic_bool _done;
  PromiseCorePtr<T> _core;

  explicit AnyCombinatorBase(size_t /*size*/, PromiseCorePtr<T>&& core) : _done{false}, _core{std::move(core)} {
  }

  bool Done() {
    return _done.load(std::memory_order_acquire);
  }

  void Combine(util::Result<T>&& result) {
    if (!_done.exchange(true, std::memory_order_acq_rel)) {
      _core->SetResult(std::move(result));
    }
  }
};

template <typename T>
class AnyCombinatorBase<T, WhenPolicy::LastFail> {
  yaclib_std::atomic_size_t _state;

 protected:
  PromiseCorePtr<T> _core;

  explicit AnyCombinatorBase(size_t size, PromiseCorePtr<T>&& core) : _state{2 * size}, _core{std::move(core)} {
  }

  bool Done() {
    return (_state.load(std::memory_order_acquire) & 1U) != 0;
  }

  void Combine(util::Result<T>&& result) {
    if (result) {
      if ((_state.exchange(1, std::memory_order_acq_rel) & 1U) == 0) {
        _core->SetResult(std::move(result));
      }
    } else if (_state.fetch_sub(2, std::memory_order_acq_rel) == 2) {
      std::move(_core)->SetResult(std::move(result));
    }
  }
};

template <typename T>
class AnyCombinatorBase<T, WhenPolicy::FirstFail> : public AnyCombinatorBase<T, WhenPolicy::None> {
  using Base = AnyCombinatorBase<T, WhenPolicy::None>;

  yaclib_std::atomic<util::ResultState> _state;
  union {
    util::detail::Unit _unit;
    std::error_code _error;
    std::exception_ptr _exception;
  };

 protected:
  explicit AnyCombinatorBase(size_t size, PromiseCorePtr<T>&& core)
      : Base{size, std::move(core)}, _state{util::ResultState::Empty}, _unit{} {
  }

  ~AnyCombinatorBase() {
    auto done = Base::_done.load(std::memory_order_acquire);
    auto state = _state.load(std::memory_order_acquire);
    if (state == util::ResultState::Error) {
      if (!done) {
        Base::_core->SetResult({_error});
      }
      _error.std::error_code::~error_code();
    } else if (state == util::ResultState::Exception) {
      if (!done) {
        Base::_core->SetResult({std::move(_exception)});
      }
      _exception.std::exception_ptr::~exception_ptr();
    }
  }

  void Combine(util::Result<T>&& result) {
    auto state = result.State();
    if (state == util::ResultState::Value) {
      return Base::Combine(std::move(result));
    }
    auto old_state = _state.load(std::memory_order_relaxed);
    if (old_state == util::ResultState::Empty &&
        _state.compare_exchange_strong(old_state, state, std::memory_order_acq_rel, std::memory_order_relaxed)) {
      if (state == util::ResultState::Error) {
        new (&_error) std::error_code{std::move(result).Error()};
      } else {
        new (&_exception) std::exception_ptr{std::move(result).Exception()};
      }
    }
  }
};

template <typename T, WhenPolicy P>
class AnyCombinator : public AnyCombinatorBase<T, P>, public InlineCore {
  using Base = AnyCombinatorBase<T, P>;

  void CallInline(InlineCore* context) noexcept final {
    if (Base::_core->GetState() != BaseCore::State::HasStop && !Base::Done()) {
      Base::Combine(std::move(static_cast<ResultCore<T>*>(context)->Get()));
    }
  }

 public:
  static std::pair<Future<T>, util::Ptr<AnyCombinator>> Make(size_t size) {
    assert(size >= 2);
    auto core = util::MakeIntrusive<detail::ResultCore<T>>();
    auto combinator = util::MakeIntrusive<AnyCombinator<T, P>>(size, core);
    return {Future<T>{std::move(core)}, std::move(combinator)};
  }

  explicit AnyCombinator(size_t size, PromiseCorePtr<T> core) : Base{size, std::move(core)} {
  }
};

template <typename T, WhenPolicy P>
using AnyCombinatorPtr = util::Ptr<AnyCombinator<T, P>>;

template <WhenPolicy P, typename T, typename ValueIt, typename RangeIt>
Future<T> WhenAnyImpl(ValueIt it, RangeIt begin, RangeIt end) {
  if (begin == end) {
    return {};
  }
  if (auto next = begin; ++next == end) {
    return std::move(*it);
  }
  const auto combinator_size = [&] {
    if constexpr (P == WhenPolicy::None || P == WhenPolicy::FirstFail) {
      return 2;
    } else {
      // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
      // Maybe the user has the size of the range, or he is satisfied with another WhenPolicy,
      // as a last resort, it is suggested to call WhenAny<WhenPolicy::LastFail>(begin, std::distance(begin, end));
      return static_cast<size_t>(end - begin);
    }
  }();
  auto [future, combinator] = AnyCombinator<T, P>::Make(combinator_size);
  for (; begin != end; ++begin) {
    it->GetCore()->SetCallbackInline(combinator);
    std::move(*it).Detach();
    ++it;
  }
  return std::move(future);
}

template <WhenPolicy P, typename T, typename... Ts>
void WhenAnyImpl(AnyCombinatorPtr<T, P>& combinator, Future<T>&& head, Future<Ts>&&... tail) {
  head.GetCore()->SetCallbackInline(combinator);
  std::move(head).Detach();
  if constexpr (sizeof...(tail) != 0) {
    WhenAnyImpl<P>(combinator, std::move(tail)...);
  }
}

}  // namespace yaclib::detail

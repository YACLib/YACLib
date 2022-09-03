#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/when_policy.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>
#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename V, typename E, WhenPolicy P /*None*/>
class AnyCombinatorBase {
 protected:
  yaclib_std::atomic_bool _done;
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t /*count*/, ResultCorePtr<V, E>&& core) noexcept
    : _done{false}, _core{std::move(core)} {
  }

  bool Done() noexcept {
    return _done.load(std::memory_order_acquire);
  }

  void Combine(Result<V, E>&& result) noexcept {
    if (!_done.exchange(true, std::memory_order_acq_rel)) {
      auto core = _core.Release();
      core->Store(std::move(result));
      core->template SetResult<false>();
    }
  }
};

template <typename V, typename E>
class AnyCombinatorBase<V, E, WhenPolicy::LastFail> {
  yaclib_std::atomic_size_t _state;

 protected:
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t count, ResultCorePtr<V, E>&& core) noexcept
    : _state{2 * count}, _core{std::move(core)} {
  }

  bool Done() noexcept {
    return (_state.load(std::memory_order_acquire) & 1U) != 0;
  }

  void Combine(Result<V, E>&& result) noexcept {
    if (result) {
      if ((_state.exchange(1, std::memory_order_acq_rel) & 1U) != 0) {
        return;
      }
    } else if (_state.fetch_sub(2, std::memory_order_acq_rel) != 2) {
      return;
    }
    auto core = _core.Release();
    core->Store(std::move(result));
    core->template SetResult<false>();
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

  ~AnyCombinatorBase() noexcept {
    auto resolve = [&](auto&& func) noexcept {
      auto state = _state.load(std::memory_order_acquire);
      if (state == ResultState::Exception) {
        func(_exception);
      } else if (state == ResultState::Error) {
        func(_error);
      }
    };
    auto core = this->_core.Release();
    if (core != nullptr) {
      resolve([&](auto& fail) noexcept {
        core->Store(std::move(fail));
        using Fail = std::remove_reference_t<decltype(fail)>;
        fail.~Fail();
      });
      core->template SetResult<false>();
    } else {
      resolve([&](auto& fail) noexcept {
        using Fail = std::remove_reference_t<decltype(fail)>;
        fail.~Fail();
      });
    }
  }

  void Combine(Result<V, E>&& result) noexcept {
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
        YACLIB_ASSERT(state == ResultState::Error);
        new (&_error) E{std::move(result).Error()};
      }
    }
  }
};

template <typename V, typename E, WhenPolicy P>
class AnyCombinator : public InlineCore, public AnyCombinatorBase<V, E, P> {
  using Base = AnyCombinatorBase<V, E, P>;
  using Base::Base;

 public:
  static auto Make(std::size_t count) {
    // TODO(MBkkt) Maybe single allocation instead of two?
    auto combine_core = MakeUnique<ResultCore<V, E>>();
    auto* raw_core = combine_core.Get();
    auto combinator = MakeShared<AnyCombinator<V, E, P>>(count, count, std::move(combine_core));
    ResultCorePtr<V, E> future_core{NoRefTag{}, raw_core};
    return std::pair{std::move(future_core), combinator.Release()};
  }

 private:
  void Here(InlineCore& caller) noexcept final {
    if (!this->Done()) {
      auto& core = static_cast<ResultCore<V, E>&>(caller);
      this->Combine(std::move(core.Get()));
    }
    DecRef();
  }
};

}  // namespace yaclib::detail

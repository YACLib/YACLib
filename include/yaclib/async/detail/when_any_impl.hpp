#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/detail/when_impl.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <cstddef>
#include <exception>
#include <limits>
#include <type_traits>
#include <utility>
#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename V, typename E, FailPolicy P /*None*/>
class AnyCombinatorBase {
  yaclib_std::atomic_bool _done;

 protected:
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t /*count*/, ResultCorePtr<V, E>&& core) noexcept
    : _done{false}, _core{std::move(core)} {
  }

  bool Combine(ResultCore<V, E>& caller) noexcept {
    if (!_done.load(std::memory_order_acquire) && !_done.exchange(true, std::memory_order_acq_rel)) {
      _core->Store(std::move(caller.Get()));
      caller.DecRef();
      return true;
    }
    caller.DecRef();
    return false;
  }
};

template <typename V, typename E>
class AnyCombinatorBase<V, E, FailPolicy::LastFail> {
  static bool DoneImpl(std::size_t value) noexcept {
    return (value & 1U) != 0;
  }

  yaclib_std::atomic_size_t _state;

 protected:
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t count, ResultCorePtr<V, E>&& core) noexcept
    : _state{2 * count}, _core{std::move(core)} {
  }

  bool Combine(ResultCore<V, E>& caller) noexcept {
    if (!DoneImpl(_state.load(std::memory_order_acquire))) {
      auto& result = caller.Get();
      if (result) {
        if (!DoneImpl(_state.exchange(1, std::memory_order_acq_rel))) {
          _core->Store(std::move(result));
          caller.DecRef();
          return true;
        }
      } else if (_state.fetch_sub(2, std::memory_order_acq_rel) == 2) {
        _core->Store(std::move(result));
        caller.DecRef();
        return true;
      }
    }
    caller.DecRef();
    return false;
  }
};

template <typename V, typename E>
class AnyCombinatorBase<V, E, FailPolicy::FirstFail> {
  static constexpr auto kDoneImpl = std::numeric_limits<std::uintptr_t>::max();

  yaclib_std::atomic_uintptr_t _state;

 protected:
  ResultCorePtr<V, E> _core;

  explicit AnyCombinatorBase(std::size_t /*count*/, ResultCorePtr<V, E>&& core) : _state{0}, _core{std::move(core)} {
  }

  ~AnyCombinatorBase() noexcept {
    const auto state = _state.load(std::memory_order_relaxed);
    if (!_core) {
      YACLIB_ASSERT(state == kDoneImpl);
      return;
    }
    YACLIB_ASSERT(state != 0);
    auto& fail = *reinterpret_cast<ResultCore<V, E>*>(state);
    _core->Store(std::move(fail.Get()));
    fail.DecRef();
    auto* core = _core.Release();
    Loop(core, core->template SetResult<false>());
  }

  bool Combine(ResultCore<V, E>& caller) noexcept {
    auto state = _state.load(std::memory_order_acquire);
    if (state != kDoneImpl) {
      auto& result = caller.Get();
      if (!result) {
        if (state != 0 || !_state.compare_exchange_strong(state, reinterpret_cast<std::uintptr_t>(&caller),
                                                          std::memory_order_acq_rel)) {
          caller.DecRef();
        }
        return false;
      }
      state = _state.exchange(kDoneImpl, std::memory_order_acq_rel);
      if (state != kDoneImpl) {
        if (state != 0) {
          auto& fail = *reinterpret_cast<ResultCore<V, E>*>(state);
          fail.DecRef();
        }
        _core->Store(std::move(caller.Get()));
        caller.DecRef();
        return true;
      }
    }
    caller.DecRef();
    return false;
  }
};

template <typename V, typename E, FailPolicy P>
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

  void AddInput(ResultCore<V, E>& input) noexcept {
    input.CallInline(*this);
  }

 private:
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if (this->Combine(DownCast<ResultCore<V, E>>(caller))) {
      auto* callback = this->_core.Release();
      DecRef();
      return WhenSetResult<SymmetricTransfer>(callback);
    }
    DecRef();
    return Noop<SymmetricTransfer>();
  }
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif
};

}  // namespace yaclib::detail

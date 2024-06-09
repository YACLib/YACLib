#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/detail/when_impl.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/order_policy.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/atomic>

namespace yaclib::detail {

// TODO(MBkkt) Unify different OrderPolicy implementation

template <OrderPolicy O /*= Same*/, typename V>
struct AllCombinatorBase {
  using FutureValue = std::vector<V>;

  yaclib_std::atomic_bool _done = false;
};

template <typename V, typename E>
struct AllCombinatorBase<OrderPolicy::Same, Result<V, E>> {
  using FutureValue = std::vector<Result<V, E>>;
};

template <>
struct AllCombinatorBase<OrderPolicy::Same, void> {
  using FutureValue = void;

  yaclib_std::atomic_bool _done = false;
};

// TODO(MBkkt) Merge _done with _ticket

template <typename V>
struct AllCombinatorBase<OrderPolicy::Fifo, V> {
  using FutureValue = std::vector<V>;

  yaclib_std::atomic_bool _done = false;
  yaclib_std::atomic_size_t _ticket = 0;
  FutureValue _results;
};

template <typename V, typename E>
struct AllCombinatorBase<OrderPolicy::Fifo, Result<V, E>> {
  using FutureValue = std::vector<Result<V, E>>;

  yaclib_std::atomic_size_t _ticket = 0;
  FutureValue _results;
};

template <>
struct AllCombinatorBase<OrderPolicy::Fifo, bool> {
  using FutureValue = std::vector<unsigned char>;

  yaclib_std::atomic_bool _done = false;
  yaclib_std::atomic_size_t _ticket = 0;
  FutureValue _results;
};

template <>
struct AllCombinatorBase<OrderPolicy::Fifo, void> {
  using FutureValue = void;

  yaclib_std::atomic_bool _done = false;
};

template <OrderPolicy O /*= Any*/, typename R, typename E>
class AllCombinator : public InlineCore, protected AllCombinatorBase<O, R> {
  using V = result_value_t<R>;
  using FutureValue = typename AllCombinatorBase<O, R>::FutureValue;
  using ResultPtr = ResultCorePtr<FutureValue, E>;

 public:
  static std::pair<ResultPtr, AllCombinator*> Make(std::size_t count) {
    if (count == 0) {
      return {nullptr, nullptr};
    }
    // TODO(MBkkt) Maybe single allocation instead of two?
    auto combine_core = MakeUnique<ResultCore<FutureValue, E>>();
    auto* raw_core = combine_core.Get();
    auto combinator = MakeShared<AllCombinator>(count, std::move(combine_core), count);
    ResultPtr future_core{NoRefTag{}, raw_core};
    return {std::move(future_core), combinator.Release()};
  }

  void AddInput(ResultCore<V, E>& input) noexcept {
    input.CallInline(*this);
  }

 protected:
  ~AllCombinator() noexcept override {
    if (std::is_same_v<R, V> && !_core) {
      return;
    }
    if constexpr (std::is_void_v<R>) {
      _core->Store(std::in_place);
    } else {
      _core->Store(std::move(this->_results));
    }
    auto* core = _core.Release();
    Loop(core, core->template SetResult<false>());
  }

  explicit AllCombinator(ResultPtr&& core, [[maybe_unused]] std::size_t count) noexcept(std::is_void_v<R>)
    : _core{std::move(core)} {
    if constexpr (!std::is_void_v<R>) {
      AllCombinatorBase<O, R>::_results.resize(count);
    }
  }

 private:
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    auto& core = DownCast<ResultCore<V, E>>(caller);
    if constexpr (std::is_same_v<R, V>) {
      if (!this->_done.load(std::memory_order_acquire) && CombineValue(std::move(core.Get()))) {
        auto* callback = _core.Release();
        Done(core);
        return WhenSetResult<SymmetricTransfer>(callback);
      }
    } else {
      const auto ticket = this->_ticket.fetch_add(1, std::memory_order_acq_rel);
      this->_results[ticket].~R();
      new (&this->_results[ticket]) R{std::move(core.Get())};
    }
    Done(core);
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

  bool CombineValue(Result<V, E>&& result) noexcept {
    const auto state = result.State();
    if (state == ResultState::Value) {
      if constexpr (!std::is_void_v<V>) {
        const auto ticket = this->_ticket.fetch_add(1, std::memory_order_acq_rel);
        this->_results[ticket] = std::move(result).Value();
      }
    } else if (!this->_done.exchange(true, std::memory_order_acq_rel)) {
      if (state == ResultState::Exception) {
        _core->Store(std::move(result).Exception());
      } else {
        YACLIB_ASSERT(state == ResultState::Error);
        _core->Store(std::move(result).Error());
      }
      return true;
    }
    return false;
  }

  void Done(ResultCore<V, E>& caller) noexcept {
    caller.DecRef();
    DecRef();
  }

  ResultPtr _core;
};

template <typename R, typename E>
class AllCombinator<OrderPolicy::Same, R, E> : public InlineCore, public AllCombinatorBase<OrderPolicy::Same, R> {
  using V = result_value_t<R>;
  using FutureValue = typename AllCombinatorBase<OrderPolicy::Same, R>::FutureValue;
  using ResultPtr = ResultCorePtr<FutureValue, E>;

 public:
  static std::pair<ResultPtr, AllCombinator*> Make(std::size_t count) {
    if (count == 0) {
      return {nullptr, nullptr};
    }
    // TODO(MBkkt) Maybe single allocation instead of two?
    auto combine_core = MakeUnique<ResultCore<FutureValue, E>>();
    auto* raw_core = combine_core.Get();
    auto combinator = MakeShared<AllCombinator>(count, std::move(combine_core), count);
    ResultPtr future_core{NoRefTag{}, raw_core};
    return {std::move(future_core), combinator.Release()};
  }

  void AddInput(ResultCore<V, E>& input) noexcept {
    _callers.push_back(&input);  // we made reserve in ctor, so noexcept
    input.CallInline(*this);
  }

 protected:
  void Clear() noexcept {
    for (auto* caller : _callers) {
      caller->DecRef();
    }
    _callers = {};
  }

  ~AllCombinator() noexcept override {
    if (std::is_same_v<R, V> && !_core) {
      Clear();
      return;
    }
    if constexpr (std::is_void_v<R>) {
      Clear();
      _core->Store(std::in_place);
    } else {
      std::vector<R> results;
      results.reserve(_callers.size());
      for (auto* caller : _callers) {
        if constexpr (std::is_same_v<R, V>) {
          results.push_back(std::move(caller->Get()).Value());
        } else {
          results.push_back(std::move(caller->Get()));
        }
        caller->DecRef();
      }
      _callers = {};
      _core->Store(std::move(results));
    }
    auto* callback = _core.Release();
    Loop(callback, callback->template SetResult<false>());
  }

  explicit AllCombinator(ResultPtr&& core, std::size_t count) : _core{std::move(core)} {
    _callers.reserve(count);
  }

 private:
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    auto& core = DownCast<ResultCore<V, E>>(caller);
    if constexpr (std::is_same_v<R, V>) {
      if (!this->_done.load(std::memory_order_acquire) && CombineValue(std::move(core.Get()))) {
        auto* callback = _core.Release();
        DecRef();
        return WhenSetResult<SymmetricTransfer>(callback);
      }
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

  bool CombineValue(Result<V, E>&& result) noexcept {
    const auto state = result.State();
    if (state != ResultState::Value && !this->_done.exchange(true, std::memory_order_acq_rel)) {
      if (state == ResultState::Exception) {
        _core->Store(std::move(result).Exception());
      } else {
        YACLIB_ASSERT(state == ResultState::Error);
        _core->Store(std::move(result).Error());
      }
      return true;
    }
    return false;
  }

  std::vector<ResultCore<V, E>*> _callers;
  ResultPtr _core;
};

}  // namespace yaclib::detail

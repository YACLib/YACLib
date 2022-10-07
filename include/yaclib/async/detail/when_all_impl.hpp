#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename V>
class AllCombinatorBase {
 protected:
  yaclib_std::atomic_size_t _ticket = 0;
  yaclib_std::atomic_bool _done = false;
  V _results;
};

template <>
class AllCombinatorBase<void> {
 protected:
  yaclib_std::atomic_bool _done = false;
};

template <typename V, typename E, typename FutureValue = std::conditional_t<std::is_void_v<V>, void, std::vector<V>>>
class AllCombinator : public InlineCore, public AllCombinatorBase<FutureValue> {
  static_assert(std::is_void_v<V> || std::is_nothrow_move_assignable_v<V>);

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

  ~AllCombinator() noexcept override {
    if (!this->_done.load(std::memory_order_acquire)) {
      auto core = _promise.Release();
      if constexpr (std::is_void_v<V>) {
        core->Store(Unit{});
      } else {
        core->Store(std::move(this->_results));
      }
      core->template SetResult<false>();
    }
    YACLIB_ASSERT(_promise == nullptr);
  }

 protected:
  explicit AllCombinator(ResultPtr&& promise, [[maybe_unused]] std::size_t count) : _promise{std::move(promise)} {
    if constexpr (!std::is_void_v<V>) {
      AllCombinatorBase<FutureValue>::_results.resize(count);
    }
  }

 private:
  void Here(BaseCore& caller) noexcept final {
    if (!this->_done.load(std::memory_order_acquire)) {
      auto& core = static_cast<ResultCore<V, E>&>(caller);
      Combine(std::move(core.Get()));
    }
    caller.DecRef();
    DecRef();
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    Here(caller);
    return yaclib_std::noop_coroutine();
  }
#endif

  void Combine(Result<V, E>&& result) noexcept {
    const auto state = result.State();
    if (state == ResultState::Value) {
      if constexpr (!std::is_void_v<V>) {
        const auto ticket = this->_ticket.fetch_add(1, std::memory_order_acq_rel);
        this->_results[ticket] = std::move(result).Value();
      }
    } else if (!this->_done.exchange(true, std::memory_order_acq_rel)) {
      auto* core = _promise.Release();
      if (state == ResultState::Exception) {
        core->Store(std::move(result).Exception());
      } else {
        YACLIB_ASSERT(state == ResultState::Error);
        core->Store(std::move(result).Error());
      }
      core->template SetResult<false>();
    }
  }

  ResultPtr _promise;
};

}  // namespace yaclib::detail

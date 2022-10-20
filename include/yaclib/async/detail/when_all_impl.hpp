#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/detail/unique_counter.hpp>
#include <yaclib/util/helper.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/atomic>

namespace yaclib::detail {

template <typename V>
struct AllCombinatorBase {
  using FutureValue = std::vector<V>;

  yaclib_std::atomic_bool _done = false;
  yaclib_std::atomic_size_t _ticket = 0;
  FutureValue _results;
};

template <typename V, typename E>
struct AllCombinatorBase<Result<V, E>> {
  using FutureValue = std::vector<Result<V, E>>;
  yaclib_std::atomic_size_t _ticket = 0;
  FutureValue _results;
};

template <>
struct AllCombinatorBase<bool> {
  using FutureValue = std::vector<unsigned char>;
  yaclib_std::atomic_bool _done = false;
  yaclib_std::atomic_size_t _ticket = 0;
  FutureValue _results;
};

template <>
struct AllCombinatorBase<void> {
  using FutureValue = void;
  yaclib_std::atomic_bool _done = false;
};

template <typename R, typename E>
class AllCombinator : public InlineCore, protected AllCombinatorBase<R> {
  using V = result_value_t<R>;
  using FutureValue = typename AllCombinatorBase<R>::FutureValue;
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
    _core.Release()->template SetResult<false>();
  }

  explicit AllCombinator(ResultPtr&& core, [[maybe_unused]] std::size_t count) noexcept(std::is_void_v<R>)
    : _core{std::move(core)} {
    if constexpr (!std::is_void_v<R>) {
      AllCombinatorBase<R>::_results.resize(count);
    }
  }

 private:
  void Here(BaseCore& caller) noexcept final {
    auto& core = static_cast<ResultCore<V, E>&>(caller);
    if constexpr (std::is_same_v<R, V>) {
      if (!this->_done.load(std::memory_order_acquire) && CombineValue(std::move(core.Get()))) {
        auto* callback = _core.Release();
        Done(core);
        callback->template SetResult<false>();
      } else {
        Done(core);
      }
    } else {
      const auto ticket = this->_ticket.fetch_add(1, std::memory_order_acq_rel);
      this->_results[ticket].~R();
      new (&this->_results[ticket]) R{std::move(core.Get())};
      Done(core);
    }
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    Here(caller);
    return yaclib_std::noop_coroutine();
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

}  // namespace yaclib::detail

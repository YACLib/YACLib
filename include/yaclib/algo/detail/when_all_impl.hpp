#pragma once

#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>

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
  yaclib_std::atomic_bool _done = false;
  yaclib_std::atomic_size_t _ticket = 0;
  V _results;
};

template <>
class AllCombinatorBase<void> {
 protected:
  yaclib_std::atomic_bool _done = false;
};

template <typename V, typename E, std::size_t N = 0,
          typename FutureValue =
            std::conditional_t<std::is_void_v<V>, void, std::conditional_t<N != 0, std::array<V, N>, std::vector<V>>>>
class AllCombinator : public InlineCore, public AllCombinatorBase<FutureValue> {
  using Base = AllCombinatorBase<FutureValue>;
  using ResultPtr = ResultCorePtr<FutureValue, E>;

 public:
  static std::pair<ResultPtr, AllCombinator*> Make(std::size_t count) {
    if constexpr (N == 0) {
      if (count == 0) {
        return {nullptr, nullptr};
      }
    }
    // TODO(MBkkt) Maybe single allocation instead of two?
    auto raw_core = new AtomicCounter<ResultCore<FutureValue, E>>{2};
    ResultPtr combine_core{NoRefTag{}, raw_core};
    ResultPtr future_core{NoRefTag{}, raw_core};
    auto combinator = new AtomicCounter<AllCombinator>{count, std::move(combine_core), count};
    return {std::move(future_core), combinator};
  }

  ~AllCombinator() override {
    if (!this->_done.load(std::memory_order_acquire)) {
      if constexpr (std::is_void_v<V>) {
        _promise->Set(Unit{});
      } else {
        _promise->Set(std::move(AllCombinatorBase<FutureValue>::_results));
      }
    }
  }

 protected:
  explicit AllCombinator(ResultPtr promise, [[maybe_unused]] std::size_t count) : _promise{std::move(promise)} {
    if constexpr (!std::is_void_v<V> && N == 0) {
      AllCombinatorBase<FutureValue>::_results.resize(count);
    }
  }

 private:
  void CallInline(InlineCore& caller, State) noexcept final {
    if (_promise->Alive() && !this->_done.load(std::memory_order_acquire)) {
      auto& core = static_cast<ResultCore<V, E>&>(caller);
      Combine(std::move(core.Get()));
    }
  }

  void Combine(Result<V, E>&& result) noexcept {
    auto const state = result.State();
    if (state == ResultState::Value) {
      if constexpr (!std::is_void_v<V>) {
        const auto ticket = AllCombinatorBase<FutureValue>::_ticket.fetch_add(1, std::memory_order_acq_rel);
        AllCombinatorBase<FutureValue>::_results[ticket] = std::move(result).Value();
      }
    } else if (!this->_done.exchange(true, std::memory_order_acq_rel)) {
      if (state == ResultState::Exception) {
        _promise->Set(std::move(result).Exception());
      } else {
        _promise->Set(std::move(result).Error());
      }
    }
  }

  ResultPtr _promise;
};

}  // namespace yaclib::detail

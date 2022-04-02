#pragma once

#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/fault/atomic.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

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

 public:
  static std::pair<Future<FutureValue, E>, AllCombinator*> Make(std::size_t size) {
    if constexpr (N == 0) {
      if (size == 0) {
        return {Future<FutureValue, E>{}, nullptr};
      }
    }
    auto raw_core = new detail::AtomicCounter<detail::ResultCore<FutureValue, E>>{2};
    IntrusivePtr combine_core{NoRefTag{}, raw_core};
    IntrusivePtr future_core{NoRefTag{}, raw_core};
    auto combinator = new detail::AtomicCounter<AllCombinator>{size, std::move(combine_core), size};
    return {Future<FutureValue, E>{std::move(future_core)}, combinator};
  }

  explicit AllCombinator(ResultCorePtr<FutureValue, E> promise, [[maybe_unused]] std::size_t size)
      : _promise{std::move(promise)} {
    if constexpr (!std::is_void_v<V> && N == 0) {
      AllCombinatorBase<FutureValue>::_results.resize(size);
    }
  }

  void CallInline(InlineCore& caller, State) noexcept final {
    if (_promise->Alive() && !Base::_done.load(std::memory_order_acquire)) {
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
    } else if (!Base::_done.exchange(true, std::memory_order_acq_rel)) {
      if (state == ResultState::Exception) {
        _promise->Set(std::move(result).Exception());
      } else {
        _promise->Set(std::move(result).Error());
      }
    }
  }

  ~AllCombinator() override {
    if (!Base::_done.load(std::memory_order_acquire)) {
      if constexpr (std::is_void_v<V>) {
        _promise->Set(Unit{});
      } else {
        _promise->Set(std::move(AllCombinatorBase<FutureValue>::_results));
      }
    }
  }

 private:
  detail::ResultCorePtr<FutureValue, E> _promise;
};

}  // namespace yaclib::detail

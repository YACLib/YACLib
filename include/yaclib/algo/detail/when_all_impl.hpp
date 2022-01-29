#pragma once

#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/fault/atomic.hpp>

#include <array>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace yaclib::detail {

template <typename T>
class AllCombinatorBase {
 protected:
  yaclib_std::atomic<bool> _done{false};
  yaclib_std::atomic<size_t> _ticket{0};
  T _results;
};

template <>
class AllCombinatorBase<void> {
 protected:
  yaclib_std::atomic<bool> _done{false};
};

template <
    typename T, size_t N = std::numeric_limits<size_t>::max(), bool IsArray = (N != std::numeric_limits<size_t>::max()),
    typename FutureValue =
        std::conditional_t<std::is_void_v<T>, void, std::conditional_t<IsArray, std::array<T, N>, std::vector<T>>>>
class AllCombinator : public InlineCore, public AllCombinatorBase<FutureValue> {
  using Base = AllCombinatorBase<FutureValue>;

 public:
  static std::pair<Future<FutureValue>, util::Ptr<AllCombinator>> Make(size_t size) {
    if constexpr (!IsArray) {
      if (size == 0) {
        return {Future<FutureValue>{}, nullptr};
      }
    }
    auto core = util::MakeIntrusive<detail::ResultCore<FutureValue>>();
    auto combinator = util::MakeIntrusive<AllCombinator>(core, size);
    return {Future<FutureValue>{std::move(core)}, std::move(combinator)};
  }

  explicit AllCombinator(PromiseCorePtr<FutureValue> promise, [[maybe_unused]] size_t size)
      : _promise{std::move(promise)} {
    if constexpr (!std::is_void_v<T> && !IsArray) {
      AllCombinatorBase<FutureValue>::_results.resize(size);
    }
  }

  void CallInline(InlineCore* context) noexcept final {
    if (_promise->GetState() != BaseCore::State::HasStop && !Base::_done.load(std::memory_order_acquire)) {
      Combine(std::move(static_cast<ResultCore<T>*>(context)->Get()));
    }
  }

  void Combine(util::Result<T>&& result) noexcept {
    auto state = result.State();
    if (state == util::ResultState::Value) {
      if constexpr (!std::is_void_v<T>) {
        const auto ticket = AllCombinatorBase<FutureValue>::_ticket.fetch_add(1, std::memory_order_acq_rel);
        AllCombinatorBase<FutureValue>::_results[ticket] = std::move(result).Value();
      }
      return;
    }
    if (!Base::_done.exchange(true, std::memory_order_acq_rel)) {
      if (state == util::ResultState::Exception) {
        _promise->Set(std::move(result).Exception());
      } else {
        _promise->Set(std::move(result).Error());
      }
    }
  }

  ~AllCombinator() override {
    if (!Base::_done.load(std::memory_order_acquire)) {
      if constexpr (std::is_void_v<T>) {
        _promise->Set(util::Unit{});
      } else {
        _promise->Set(std::move(AllCombinatorBase<FutureValue>::_results));
      }
    }
  }

 private:
  detail::PromiseCorePtr<FutureValue> _promise;
};

template <size_t N, typename T, typename... Ts>
void WhenAllImpl(util::Ptr<AllCombinator<T, N>>& combinator, Future<T>&& head, Future<Ts>&&... tail) {
  head.GetCore()->SetCallbackInline(combinator);
  std::move(head).Detach();
  if constexpr (sizeof...(tail) != 0) {
    WhenAllImpl(combinator, std::move(tail)...);
  }
}

}  // namespace yaclib::detail

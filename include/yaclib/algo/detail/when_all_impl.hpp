#pragma once

#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>

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
  static std::pair<Future<FutureValue>, util::Ptr<AllCombinator>> Make(size_t size = 0) {
    auto [future, promise] = MakeContract<FutureValue>();
    if constexpr (!IsArray) {
      if (size == 0) {
        if constexpr (std::is_void_v<T>) {
          std::move(promise).Set();
        } else {
          std::move(promise).Set(std::vector<T>{});
        }
        return {std::move(future), nullptr};
      }
    }
    return {std::move(future), util::MakeIntrusive<AllCombinator>(std::move(promise), size)};
  }

  explicit AllCombinator(Promise<FutureValue> promise, [[maybe_unused]] size_t size = 0)
      : _promise{std::move(promise)} {
    if constexpr (!std::is_void_v<T> && !IsArray) {
      AllCombinatorBase<FutureValue>::_results.resize(size);
    }
  }

  void CallInline(InlineCore* context) noexcept final {
    if (_promise.GetCore()->GetState() != BaseCore::State::HasStop) {
      Combine(std::move(static_cast<ResultCore<T>*>(context)->Get()));
    }
  }

  void Combine(util::Result<T>&& result) noexcept(std::is_void_v<T> || std::is_nothrow_move_assignable_v<T>) {
    if (Base::_done.load(std::memory_order_acquire)) {
      return;
    }
    auto state = result.State();
    if (state == util::ResultState::Value) {
      if constexpr (!std::is_void_v<T>) {
        const auto ticket = AllCombinatorBase<FutureValue>::_ticket.fetch_add(1, std::memory_order_acq_rel);
        AllCombinatorBase<FutureValue>::_results[ticket] = std::move(result).Value();
      }
      return;
    }
    if (Base::_done.exchange(true, std::memory_order_acq_rel)) {
      return;
    }
    if (state == util::ResultState::Error) {
      std::move(_promise).Set(std::move(result).Error());
    } else {
      std::move(_promise).Set(std::move(result).Exception());
    }
  }

  ~AllCombinator() override {
    if (!Base::_done.load(std::memory_order_acquire)) {
      if constexpr (!std::is_void_v<T>) {
        std::move(_promise).Set(std::move(AllCombinatorBase<FutureValue>::_results));
      } else {
        std::move(_promise).Set();
      }
    }
  }

 private:
  Promise<FutureValue> _promise;
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

#pragma once

#include <yaclib/async/detail/result_core.hpp>

namespace yaclib::detail {

template <typename Ret, typename Arg, typename E, typename InvokeType, bool Run = false>
class Core : public ResultCore<Ret, E> {
  using Base = ResultCore<Ret, E>;

 public:
  template <typename... T>
  explicit Core(T&&... t) : _functor{std::forward<T>(t)...} {
  }

 private:
  void Call() noexcept final {
    if (!Base::Alive()) {
      return Base::Cancel();
    }
    if constexpr (Run) {
      _functor.Wrapper(*this, Unit{});
    } else {
      _functor.Wrapper(*this, std::move(static_cast<ResultCore<Arg, E>&>(*Base::_caller).Get()));
    }
  }

  void CallInline(InlineCore* caller, [[maybe_unused]] InlineCore::State state) noexcept final {
    if (!Base::Alive()) {
      return;  // Don't need to call Cancel, because we call Clean after CallInline and our _caller is nullptr
    }
    if constexpr (InvokeType::kIsAsync) {
      if (state == InlineCore::State::HasAsyncCallback) {
        return Base::Set(std::move(static_cast<ResultCore<Ret, E>*>(caller)->Get()));
      }
    }
    _functor.Wrapper(*this, std::move(static_cast<ResultCore<Arg, E>*>(caller)->Get()));
  }

  InvokeType _functor;
};

}  // namespace yaclib::detail

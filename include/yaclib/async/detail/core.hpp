#pragma once

#include <yaclib/async/detail/result_core.hpp>

namespace yaclib::detail {

template <typename Ret, typename InvokeType, typename Arg, bool Run = false>
class Core : public ResultCore<Ret> {
  using Base = ResultCore<Ret>;

 public:
  template <typename... T>
  explicit Core(T&&... t) : _functor{std::forward<T>(t)...} {
  }

 private:
  void Call() noexcept final {
    if (Base::GetState() == BaseCore::State::HasStop) {
      return Base::Cancel();
    }
    if constexpr (Run) {
      Base::Set(_functor.Wrapper(util::Result<Arg>{util::Unit{}}));
    } else {
      Base::Set(_functor.Wrapper(std::move(static_cast<ResultCore<Arg>&>(*Base::_caller).Get())));
    }
  }

  void CallInline(InlineCore* caller) noexcept final {
    if (Base::GetState() == BaseCore::State::HasStop) {
      return;  // Don't need to call Cancel, because we call Clean after CallInline and our _caller is nullptr
    }
    Base::Set(_functor.Wrapper(std::move(static_cast<ResultCore<Arg>*>(caller)->Get())));
  }

  InvokeType _functor;
};

}  // namespace yaclib::detail

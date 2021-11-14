#pragma once

#include <yaclib/async/detail/result_core.hpp>

namespace yaclib::detail {

template <typename Ret, typename InvokeType, typename Arg>
class Core : public ResultCore<Ret> {
  using Base = ResultCore<Ret>;

 public:
  template <typename... T>
  explicit Core(T&&... t) : _functor{std::forward<T>(t)...} {
  }

 private:
  void Call() noexcept final {
    if (Base::GetState() == BaseCore::State::HasStop) {
      Base::Cancel();
      return;
    }
    auto* caller = static_cast<ResultCore<Arg>*>(Base::_caller.Get());
    if constexpr (std::is_void_v<Arg>) {
      if (!caller) {
        Base::SetResult(_functor.Wrapper(util::Result<Arg>::Default()));
        return;
      }
    }
    Base::SetResult(_functor.Wrapper(std::move(caller->Get())));
  }

  void CallInline(void* caller) noexcept final {
    if (Base::GetState() == BaseCore::State::HasStop) {
      // Don't need to call Cancel, because we call Clean after CallInline and our _caller is nullptr
      return;
    }
    Base::SetResult(_functor.Wrapper(std::move(static_cast<ResultCore<Arg>*>(caller)->Get())));
  }

  InvokeType _functor;
};

}  // namespace yaclib::detail

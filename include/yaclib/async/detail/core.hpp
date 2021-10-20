#pragma once

#include <yaclib/async/detail/result_core.hpp>

#include <cassert>

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
    if (Base::_state.load(std::memory_order_acquire) == Base::State::Stopped) {
      Base::Cancel();
      return;
    }
    auto ret = [&] {
      auto* caller = static_cast<ResultCore<Arg>*>(Base::_caller.Get());
      if constexpr (std::is_void_v<Arg>) {
        if (!caller) {
          return util::Result<Arg>::Default();
        }
      }
      return std::move(caller->Get());
    }();
    Base::_caller = nullptr;
    Base::SetResult(_functor.Wrapper(std::move(ret)));
  }

  InvokeType _functor;
};

template <typename Result>
class Core<Result, void, void> : public ResultCore<Result> {
  void Call() noexcept final {
    assert(false);  // This class using only via promise
  }
};

extern template class Core<void, void, void>;

}  // namespace yaclib::detail

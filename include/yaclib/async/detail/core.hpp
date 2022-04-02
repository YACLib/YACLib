#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/config.hpp>

namespace yaclib::detail {

enum class CoreType {
  Run,
  Then,
  Subscribe,
};

template <CoreType Type, typename V, typename E>
using ResultCoreT = std::conditional_t<Type == CoreType::Subscribe, ResultCore<void, void>, ResultCore<V, E>>;

template <typename Ret, typename Arg, typename E, typename Wrapper, CoreType Type>
class Core : public ResultCoreT<Type, Ret, E> {
 public:
  using Base = ResultCoreT<Type, Ret, E>;

  template <typename Functor>
  explicit Core(Functor&& functor) : _wrapper{std::forward<Functor>(functor)} {
  }

 private:
  void Call() noexcept final {
    if (!Base::Alive()) {
      return Base::Cancel();
    }
    if constexpr (Type == CoreType::Run) {
      _wrapper.Call(*this, Unit{});
    } else {
      auto& core = static_cast<ResultCore<Arg, E>&>(*Base::_caller);
      _wrapper.Call(*this, std::move(core.Get()));
    }
  }

  void CallInline(InlineCore& caller, [[maybe_unused]] InlineCore::State state) noexcept final {
    if (!Base::Alive()) {
      return;  // Don't need to call Cancel, because we call Clean after CallInline and our _caller is nullptr
    }
    if constexpr (Wrapper::kIsAsync) {
      if (state == InlineCore::State::HasAsyncCallback) {
        auto& core = static_cast<ResultCore<Ret, E>&>(caller);
        return Base::Set(std::move(core.Get()));
      }
    }
    auto& core = static_cast<ResultCore<Arg, E>&>(caller);
    _wrapper.Call(*this, std::move(core.Get()));
  }

  Wrapper _wrapper;
};

}  // namespace yaclib::detail

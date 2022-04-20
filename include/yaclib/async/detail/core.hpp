#pragma once

#include <yaclib/async/detail/result_core.hpp>

namespace yaclib::detail {

enum class CoreType : char {
  Run = 0,
  Then = 1,
  Detach = 2,
};

template <CoreType Type, typename V, typename E>
using ResultCoreT = std::conditional_t<Type == CoreType::Detach, ResultCore<void, void>, ResultCore<V, E>>;

template <typename Ret, typename Arg, typename E, typename Wrapper, CoreType Type>
class Core : public ResultCoreT<Type, Ret, E> {
 public:
  using Base = ResultCoreT<Type, Ret, E>;

  template <typename Functor>
  explicit Core(Functor&& functor) : _wrapper{std::forward<Functor>(functor)} {
  }

 private:
  void Call() noexcept final {
    if (!this->Alive()) {
      return this->Cancel();
    }
    if constexpr (Type == CoreType::Run) {
      _wrapper.Call(*this, Unit{});
    } else {
      auto& core = static_cast<ResultCore<Arg, E>&>(*this->_caller);
      _wrapper.Call(*this, std::move(core.Get()));
    }
    this->DecRef();
  }

  void CallInline(InlineCore& caller, [[maybe_unused]] InlineCore::State state) noexcept final {
    if (!this->Alive()) {
      return this->Set(StopTag{});
    }
    if constexpr (Wrapper::kIsAsync) {
      if (state == InlineCore::State::HasAsyncCallback) {
        auto& core = static_cast<ResultCore<Ret, E>&>(caller);
        return this->Set(std::move(core.Get()));
      }
    }
    auto& core = static_cast<ResultCore<Arg, E>&>(caller);
    _wrapper.Call(*this, std::move(core.Get()));
  }

  Wrapper _wrapper;
};

}  // namespace yaclib::detail

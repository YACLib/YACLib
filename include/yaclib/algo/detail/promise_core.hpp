#pragma once

#include <yaclib/algo/detail/func_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/log.hpp>

#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename V, typename E, typename Func>
class PromiseCore : public ResultCore<V, E>, public FuncCore<Func> {
  using F = FuncCore<Func>;
  using Invoke = typename F::Invoke;
  using Storage = typename F::Storage;

 public:
  using Base = ResultCore<V, E>;

  explicit PromiseCore(Func&& f) : F{std::forward<Func>(f)} {
  }

 private:
  void Call() noexcept final {
    Promise<V, E> promise{ResultCorePtr<V, E>{NoRefTag{}, this}};
    try {
      // We need to move func with capture on stack, because promise can be Set before func return
      static_assert(std::is_nothrow_move_constructible_v<Storage>);
      auto func = std::move(this->_func.storage);
      this->_func.storage.~Storage();
      std::forward<Invoke>(func)(std::move(promise));
    } catch (...) {
      if (promise.Valid()) {
        std::move(promise).Set(std::current_exception());
      } else {
        // ignore it, because promise already used
        YACLIB_WARN(true, "Your exception will be ignored, you probably move promise too early");
      }
    }
  }

  void Drop() noexcept final {
    this->_func.storage.~Storage();
    this->Store(StopTag{});
    Loop(this, this->template SetResult<false>());
  }
};

}  // namespace yaclib::detail

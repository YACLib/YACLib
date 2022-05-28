#pragma once

#include <yaclib/config.hpp>

#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename Func>
class SafeCall {
 public:
  using Store = std::decay_t<Func>;
  using Invoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Func>>, Store, Func>;

  explicit SafeCall(Store&& f) noexcept(std::is_nothrow_move_constructible_v<Store>) : _func{std::move(f)} {
  }

  explicit SafeCall(const Store& f) noexcept(std::is_nothrow_copy_constructible_v<Store>) : _func{f} {
  }

 protected:
  void Call() noexcept {
    if constexpr (std::is_nothrow_invocable_v<Invoke>) {  // TODO(MBkkt) Is it really necessary?
      std::forward<Invoke>(_func)();
    } else {
      try {
        std::forward<Invoke>(_func)();
      } catch (...) {
        // TODO(MBkkt) Special macro for user callback function?
      }
    }
  }

 private:
  YACLIB_NO_UNIQUE_ADDRESS Store _func;
};

}  // namespace yaclib::detail

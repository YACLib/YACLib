#pragma once

#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename Func>
class SafeCall {
  using Store = std::decay_t<Func>;
  using Invoke = std::conditional_t<std::is_function_v<std::remove_reference_t<Func>>, Store, Func>;

 public:
  explicit SafeCall(Store&& f) : _func{std::move(f)} {
  }

  explicit SafeCall(const Store& f) : _func{f} {
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
  Store _func;
};

}  // namespace yaclib::detail

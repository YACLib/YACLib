#pragma once

#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename Invoke>
class SafeCall {
  using Store = std::remove_cv_t<std::remove_reference_t<Invoke>>;

 public:
  explicit SafeCall(Store&& functor) : _functor{std::move(functor)} {
  }

  explicit SafeCall(const Store& functor) : _functor{functor} {
  }

 protected:
  void Call() noexcept {
    if constexpr (std::is_nothrow_invocable_v<Invoke>) {  // TODO(MBkkt) Is it really necessary?
      std::forward<Invoke>(_functor)();
    } else {
      try {
        std::forward<Invoke>(_functor)();
      } catch (...) {
        // TODO(MBkkt) Special macro for user callback function?
      }
    }
  }

 private:
  Store _functor;
};

}  // namespace yaclib::detail

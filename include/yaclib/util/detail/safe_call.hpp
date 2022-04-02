#pragma once

#include <yaclib/config.hpp>

#include <type_traits>
#include <utility>

namespace yaclib::detail {

template <typename Interface, typename Functor>
class SafeCall : public Interface {
 public:
  using FunctorStore = std::remove_cv_t<std::remove_reference_t<Functor>>;

  explicit SafeCall(FunctorStore&& functor) : _functor{std::move(functor)} {
  }

  explicit SafeCall(const FunctorStore& functor) : _functor{functor} {
  }

 private:
  void Call() noexcept final {
    if constexpr (std::is_nothrow_invocable_v<Functor>) {
      std::forward<Functor>(_functor)();
    } else {
      try {
        std::forward<Functor>(_functor)();
      } catch (...) {
        // TODO(MBkkt) Special macro for user callback function?
      }
    }
  }

  FunctorStore _functor;
};

}  // namespace yaclib::detail

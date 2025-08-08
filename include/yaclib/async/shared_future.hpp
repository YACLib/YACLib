#pragma once

#include <yaclib/algo/detail/shared_core.hpp>

namespace yaclib {

template <typename V, typename E>
class SharedFuture final {
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "SharedFuture cannot be instantiated with same V and E, because it's ambiguous");
  static_assert(std::is_copy_constructible_v<Result<V, E>>, "Result should be copyable");

 public:
  SharedFuture() = default;

  [[nodiscard]] bool Valid() const noexcept {
    return _core != nullptr;
  }

  [[nodiscard]] bool Ready() const noexcept {
    YACLIB_ASSERT(Valid());
    return !_core->Empty();
  }

  [[nodiscard]] const Result<V, E>& Get() const noexcept {
    YACLIB_ASSERT(Valid());
    Wait(*this);
    return _core->Get();
  }

  [[nodiscard]] const Result<V, E>& Touch() const noexcept {
    YACLIB_ASSERT(Valid());
    YACLIB_ASSERT(Ready());
    return _core->Get();
  }

  [[nodiscard]] const detail::SharedCorePtr<V, E>& GetCore() const noexcept {
    return _core;
  }

  [[nodiscard]] detail::SharedHandle GetBaseHandle() const noexcept {
    return detail::SharedHandle{*_core};
  }

  /**
   * Part of unsafe but internal API
   */
  explicit SharedFuture(detail::SharedCorePtr<V, E> core) noexcept : _core(std::move(core)) {
  }

 private:
  detail::SharedCorePtr<V, E> _core;
};

}  // namespace yaclib

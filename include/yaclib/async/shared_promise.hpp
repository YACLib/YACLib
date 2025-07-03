#pragma once

#include <yaclib/algo/detail/shared_core.hpp>

namespace yaclib {

template <typename V, typename E>
class SharedPromise final {
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Future cannot be instantiated with same V and E, because it's ambiguous");
  static_assert(std::is_copy_constructible_v<Result<V, E>>, "Result should be copyable");

 public:
  SharedPromise() noexcept = default;

  SharedPromise(const SharedPromise& other) = delete;
  SharedPromise& operator=(const SharedPromise& other) = delete;

  SharedPromise(SharedPromise&& other) noexcept = default;
  SharedPromise& operator=(SharedPromise&& other) noexcept = default;

  [[nodiscard]] bool Valid() const& noexcept {
    return _core != nullptr;
  }

  template <typename... Args>
  void Set(Args&&... args) && {
    YACLIB_ASSERT(Valid());
    if constexpr (sizeof...(Args) == 0) {
      _core->Set(std::in_place);
    } else {
      _core->Set(std::forward<Args>(args)...);
    }
    _core = nullptr;
  }

  ~SharedPromise() {
    if (Valid()) {
      std::move(*this).Set(StopTag{});
    }
  }

  /**
   * Part of unsafe but internal API
   */
  explicit SharedPromise(detail::SharedCorePtr<V, E> core) noexcept : _core(std::move(core)) {
  }

 private:
  detail::SharedCorePtr<V, E> _core;
};

}  // namespace yaclib

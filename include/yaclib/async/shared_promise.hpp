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

  [[nodiscard]] bool Valid() const noexcept {
    return _core != nullptr;
  }

  template <typename... Args>
  void Set(Args&&... args) && {
    YACLIB_ASSERT(Valid());

    detail::InlineCore* head = nullptr;
    if constexpr (sizeof...(Args) == 0) {
      head = _core->Store(std::in_place);
    } else {
      head = _core->Store(std::forward<Args>(args)...);
    }

    auto released = _core.Release();
    released->FulfillQueue(head);
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

  [[nodiscard]] detail::SharedCorePtr<V, E>& GetCore() noexcept {
    return _core;
  }

 private:
  detail::SharedCorePtr<V, E> _core;
};

}  // namespace yaclib

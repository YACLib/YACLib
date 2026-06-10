#pragma once

#include <yaclib/algo/detail/shared_core.hpp>

namespace yaclib {

template <typename V, typename T>
class SharedPromise final {
  static_assert(Check<V>(), "V should be valid");
  static_assert(!std::is_same_v<V, typename T::Error>,
                "V cannot be the same as the trait Error type, because callback dispatch would be ambiguous");
  static_assert(std::is_copy_constructible_v<wrap_void_t<V>>, "Result should be copyable");

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

    if constexpr (sizeof...(Args) == 0) {
      _core->Store(Unit{});
    } else {
      _core->Store(std::forward<Args>(args)...);
    }

    auto released = _core.Release();
    // The result will always be null
    std::ignore = released->template SetResult<false>();
  }

  ~SharedPromise() {
    if (Valid()) {
      std::move(*this).Set(StopTag{});
    }
  }

  /**
   * Part of unsafe but internal API
   */
  explicit SharedPromise(detail::SharedCorePtr<V, T> core) noexcept : _core(std::move(core)) {
  }

  [[nodiscard]] detail::SharedCorePtr<V, T>& GetCore() noexcept {
    return _core;
  }

 private:
  detail::SharedCorePtr<V, T> _core;
};

}  // namespace yaclib

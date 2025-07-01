#pragma once

#include <yaclib/async/detail/shared_core.hpp>

namespace yaclib {

template <typename V, typename E>
class SharedPromise final {
 public:
  SharedPromise() noexcept = default;

  explicit SharedPromise(detail::SharedCorePtr<V, E> core) noexcept : _core(std::move(core)) {
  }

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

 private:
  detail::SharedCorePtr<V, E> _core;
};

}  // namespace yaclib

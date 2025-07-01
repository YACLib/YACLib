#pragma once

#include <yaclib/async/detail/shared_core.hpp>

namespace yaclib {

template <typename V, typename E>
class SharedFuture final {
 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Future cannot be instantiated with same V and E, because it's ambiguous");
  static_assert(std::is_copy_constructible_v<Result<V, E>>, "Result should be copyable");

  SharedFuture() : _core(nullptr) {
  }

  explicit SharedFuture(detail::SharedCorePtr<V, E> core) : _core(std::move(core)) {
  }

  [[nodiscard]] bool Valid() const& noexcept {
    return _core != nullptr;
  }

  [[nodiscard]] Result<V, E> Get() const {
    YACLIB_ASSERT(Valid());
    return _core->Get();
  }

  Future<V, E> GetFuture() const {
    YACLIB_ASSERT(Valid());
    return _core->GetFuture();
  }

  FutureOn<V, E> GetFutureOn(IExecutor& e) const {
    YACLIB_ASSERT(Valid());
    return _core->GetFutureOn(e);
  }

  void Attach(Promise<V, E>&& p) const {
    YACLIB_ASSERT(Valid());
    _core->Attach(std::move(p));
  }

 private:
  detail::SharedCorePtr<V, E> _core;
};

}  // namespace yaclib

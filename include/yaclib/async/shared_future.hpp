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

  [[nodiscard]] bool Valid() const& noexcept {
    return _core != nullptr;
  }

  [[nodiscard]] Result<V, E> Get() const {
    YACLIB_ASSERT(Valid());
    return GetFuture().Get();
  }

  Future<V, E> GetFuture() const {
    YACLIB_ASSERT(Valid());
    auto [f, p] = MakeContract<V, E>();
    _core->Attach(std::move(p));
    return std::move(f);
  }

  FutureOn<V, E> GetFutureOn(IExecutor& e) const {
    YACLIB_ASSERT(Valid());
    auto [f, p] = MakeContractOn<V, E>(e);
    _core->Attach(std::move(p));
    return std::move(f);
  }

  void Attach(Promise<V, E>&& p) const {
    YACLIB_ASSERT(Valid());
    _core->Attach(std::move(p));
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

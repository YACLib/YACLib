#pragma once

#include <yaclib/algo/detail/core.hpp>
#include <yaclib/algo/detail/shared_core.hpp>
#include <yaclib/async/wait.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename E>
class SharedFutureBase {
  using CoreType = detail::CoreType;

 public:
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "SharedFuture cannot be instantiated with same V and E, because it's ambiguous");
  static_assert(std::is_copy_constructible_v<Result<V, E>>, "Result should be copyable");

  SharedFutureBase() = default;

  [[nodiscard]] bool Valid() const noexcept {
    return _core != nullptr;
  }

  [[nodiscard]] bool Ready() const noexcept {
    YACLIB_ASSERT(Valid());
    return !_core->Empty();
  }

  [[nodiscard]] Result<V, E> Get() && noexcept {
    YACLIB_ASSERT(Valid());
    Wait(*this);
    if (_core->GetRef() == 1) {
      return std::move(_core->Get());
    } else {
      return _core->Get();
    }
  }

  void Get() const&& = delete;

  [[nodiscard]] const Result<V, E>& Get() const& noexcept {
    YACLIB_ASSERT(Valid());
    Wait(*this);
    return _core->Get();
  }

  [[nodiscard]] Result<V, E> Touch() && noexcept {
    YACLIB_ASSERT(Valid());
    YACLIB_ASSERT(Ready());
    if (_core->GetRef() == 1) {
      return std::move(_core->Get());
    } else {
      return _core->Get();
    }
  }

  void Touch() const&& = delete;

  [[nodiscard]] const Result<V, E>& Touch() const& noexcept {
    YACLIB_ASSERT(Valid());
    YACLIB_ASSERT(Ready());
    return _core->Get();
  }

  template <typename Func>
  [[nodiscard]] /*FutureOn*/ auto Then(IExecutor& e, Func&& f) const {
    YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
                "better way is use ThenInline(...) instead of Then(MakeInline(), ...)");
    static constexpr auto CoreT = CoreType::ToUnique | CoreType::Call;
    return detail::SetCallback<CoreT, true>(_core, &e, std::forward<Func>(f));
  }

  void Detach() && noexcept {
    _core = nullptr;
  }

  template <typename Func>
  void SubscribeInline(Func&& f) const {
    static constexpr auto CoreT = CoreType::Detach;
    detail::SetCallback<CoreT, false>(_core, nullptr, std::forward<Func>(f));
  }

  template <typename Func>
  void Subscribe(IExecutor& e, Func&& f) const {
    YACLIB_WARN(e.Tag() == IExecutor::Type::Inline,
                "better way is use SubscribeInline(...) instead of Subscribe(MakeInline(), ...)");
    static constexpr auto CoreT = CoreType::Detach | CoreType::Call;
    detail::SetCallback<CoreT, true>(_core, &e, std::forward<Func>(f));
  }

  [[nodiscard]] detail::SharedCorePtr<V, E>& GetCore() noexcept {
    return _core;
  }

  [[nodiscard]] const detail::SharedCorePtr<V, E>& GetCore() const noexcept {
    return _core;
  }

  using Handle = detail::SharedHandle;
  using Core = detail::SharedCore<V, E>;

  [[nodiscard]] detail::SharedHandle GetHandle() const noexcept {
    return detail::SharedHandle{*_core};
  }

 protected:
  explicit SharedFutureBase(detail::SharedCorePtr<V, E> core) noexcept : _core{std::move(core)} {
  }

  detail::SharedCorePtr<V, E> _core;
};

extern template class SharedFutureBase<void, StopError>;

template <typename V, typename E>
class SharedFuture final : public SharedFutureBase<V, E> {
  using CoreType = detail::CoreType;
  using Base = SharedFutureBase<V, E>;

 public:
  using Base::Base;

  SharedFuture(detail::SharedCorePtr<V, E> core) noexcept : Base{std::move(core)} {
  }

  template <typename Func>
  [[nodiscard]] /*Future*/ auto ThenInline(Func&& f) const {
    static constexpr auto CoreT = CoreType::ToUnique;
    return detail::SetCallback<CoreT, false>(this->_core, nullptr, std::forward<Func>(f));
  }
};

extern template class SharedFuture<>;

template <typename V, typename E>
class SharedFutureOn final : public SharedFutureBase<V, E> {
  using CoreType = detail::CoreType;
  using Base = SharedFutureBase<V, E>;

 public:
  using Base::Base;
  using Base::Detach;
  using Base::Then;

  SharedFutureOn(detail::SharedCorePtr<V, E> core) noexcept : Base{std::move(core)} {
  }

  [[nodiscard]] SharedFuture<V, E> On(std::nullptr_t) && noexcept {
    return {std::move(this->_core)};
  }

  template <typename Func>
  [[nodiscard]] /*FutureOn*/ auto ThenInline(Func&& f) const {
    static constexpr auto CoreT = CoreType::ToUnique;
    return detail::SetCallback<CoreT, true>(this->_core, nullptr, std::forward<Func>(f));
  }

  template <typename Func>
  [[nodiscard]] /*FutureOn*/ auto Then(Func&& f) const {
    static constexpr auto CoreT = CoreType::ToUnique | CoreType::Call;
    return detail::SetCallback<CoreT, true>(this->_core, nullptr, std::forward<Func>(f));
  }

  template <typename Func>
  void Subscribe(Func&& f) const {
    static constexpr auto CoreT = CoreType::Detach | CoreType::Call;
    detail::SetCallback<CoreT, false>(this->_core, nullptr, std::forward<Func>(f));
  }
};

extern template class SharedFutureOn<>;

}  // namespace yaclib

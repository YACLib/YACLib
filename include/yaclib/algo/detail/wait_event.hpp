#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/cast.hpp>
#include <yaclib/util/detail/set_deleter.hpp>

namespace yaclib::detail {

template <typename Derived>
struct CallCallback : InlineCore {
  CallCallback& GetCall() noexcept {
    return *this;
  }

 private:
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl() noexcept {
    DownCast<Derived>(*this).Sub(1);
    return Noop<SymmetricTransfer>();
  }
  [[nodiscard]] InlineCore* Here(InlineCore& /*caller*/) noexcept final {
    return Impl<false>();
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& /*caller*/) noexcept final {
    return Impl<true>();
  }
#endif
};

template <typename Derived>
struct DropCallback : InlineCore {
  DropCallback& GetDrop() noexcept {
    return *this;
  }

 private:
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    caller.DecRef();
    DownCast<Derived>(*this).Sub(1);
    return Noop<SymmetricTransfer>();
  }
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif
};

template <typename Event, template <typename...> typename Counter, template <typename...> typename... Callbacks>
struct MultiEvent final : Counter<Event, SetDeleter>, Callbacks<MultiEvent<Event, Counter, Callbacks...>>... {
  using Counter<Event, SetDeleter>::Counter;
};

}  // namespace yaclib::detail

#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/detail/set_deleter.hpp>

namespace yaclib::detail {

template <typename Derived>
struct CallCallback : InlineCore {
  CallCallback& GetCall() noexcept {
    return *this;
  }

 private:
  void Here(BaseCore& /*caller*/) noexcept final {
    static_cast<Derived&>(*this).Sub(1);
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    Here(caller);
    return yaclib_std::noop_coroutine();
  }
#endif
};

template <typename Derived>
struct DropCallback : InlineCore {
  DropCallback& GetDrop() noexcept {
    return *this;
  }

 private:
  void Here(BaseCore& caller) noexcept final {
    caller.DecRef();
    static_cast<Derived&>(*this).Sub(1);
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    Here(caller);
    return yaclib_std::noop_coroutine();
  }
#endif
};

template <typename Event, template <typename...> typename Counter, template <typename...> typename... Callbacks>
struct MultiEvent final : Counter<Event, SetDeleter>, Callbacks<MultiEvent<Event, Counter, Callbacks...>>... {
  using Counter<Event, SetDeleter>::Counter;
};

}  // namespace yaclib::detail

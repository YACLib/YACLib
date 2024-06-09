#pragma once

#include <memory>
#include <type_traits>

namespace yaclib {

template <typename To, typename From>
constexpr auto* UpCast(From* from) noexcept {
  using RawTo = std::remove_const_t<To>;
  using RawFrom = std::remove_const_t<From>;
  static_assert(!std::is_pointer_v<RawTo>, "'To' shouldn't be pointer");
  static_assert(!std::is_reference_v<RawTo>, "'To' shouldn't be reference");
  using CastTo = std::conditional_t<std::is_const_v<From>, const RawTo, To>;
  static_assert(!std::is_same_v<RawTo, RawFrom>);
  static_assert(std::is_base_of_v<RawTo, RawFrom>);
  return static_cast<CastTo*>(from);
}

template <typename To, typename From, typename = std::enable_if_t<!std::is_pointer_v<From>>>
constexpr auto& UpCast(From& from) noexcept {
  return *UpCast<To>(std::addressof(from));
}

template <typename To, typename From>
constexpr auto* DownCast(From* from) noexcept {
  using RawTo = std::remove_const_t<To>;
  using RawFrom = std::remove_const_t<From>;
  static_assert(!std::is_pointer_v<RawTo>, "'To' shouldn't be pointer");
  static_assert(!std::is_reference_v<RawTo>, "'To' shouldn't be reference");
  using CastTo = std::conditional_t<std::is_const_v<From>, const RawTo, To>;
  static_assert(!std::is_same_v<RawFrom, RawTo>);
  static_assert(std::is_base_of_v<RawFrom, RawTo>);
  YACLIB_ASSERT(from == nullptr || dynamic_cast<CastTo*>(from) != nullptr);
  return static_cast<CastTo*>(from);
}

template <typename To, typename From>
constexpr auto& DownCast(From& from) noexcept {
  return *DownCast<To>(std::addressof(from));
}

}  // namespace yaclib

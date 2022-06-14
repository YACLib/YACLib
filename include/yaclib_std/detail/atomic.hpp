#pragma once

#if YACLIB_FAULT_ATOMIC == 2
#  include <yaclib/fault/detail/atomic.hpp>
#  include <yaclib/fault/detail/fiber/atomic.hpp>

#  include <atomic>

namespace yaclib_std {

template <typename T>
using atomic = yaclib::detail::Atomic<yaclib::detail::fiber::Atomic<T>, T>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_ATOMIC == 1
#  include <yaclib/fault/detail/atomic.hpp>

#  include <atomic>

namespace yaclib_std {

template <typename T>
using atomic = yaclib::detail::Atomic<std::atomic<T>, T>;

}  // namespace yaclib_std
#else
#  include <atomic>

namespace yaclib_std {

using std::atomic;

}  // namespace yaclib_std
#endif

#include <cstddef>
#include <cstdint>

namespace yaclib_std {

using atomic_bool = atomic<bool>;
using atomic_char = atomic<char>;
using atomic_schar = atomic<signed char>;
using atomic_uchar = atomic<unsigned char>;
using atomic_short = atomic<short>;
using atomic_ushort = atomic<unsigned short>;
using atomic_int = atomic<int>;
using atomic_uint = atomic<unsigned int>;
using atomic_long = atomic<long>;
using atomic_ulong = atomic<unsigned long>;
using atomic_llong = atomic<long long>;
using atomic_ullong = atomic<unsigned long long>;
using atomic_char16_t = atomic<char16_t>;
using atomic_char32_t = atomic<char32_t>;
using atomic_wchar_t = atomic<wchar_t>;

using atomic_int_least8_t = atomic<std::int_least8_t>;
using atomic_uint_least8_t = atomic<std::uint_least8_t>;
using atomic_int_least16_t = atomic<std::int_least16_t>;
using atomic_uint_least16_t = atomic<std::uint_least16_t>;
using atomic_int_least32_t = atomic<std::int_least32_t>;
using atomic_uint_least32_t = atomic<std::uint_least32_t>;
using atomic_int_least64_t = atomic<std::int_least64_t>;
using atomic_uint_least64_t = atomic<std::uint_least64_t>;

using atomic_int_fast8_t = atomic<std::int_fast8_t>;
using atomic_uint_fast8_t = atomic<std::uint_fast8_t>;
using atomic_int_fast16_t = atomic<std::int_fast16_t>;
using atomic_uint_fast16_t = atomic<std::uint_fast16_t>;
using atomic_int_fast32_t = atomic<std::int_fast32_t>;
using atomic_uint_fast32_t = atomic<std::uint_fast32_t>;
using atomic_int_fast64_t = atomic<std::int_fast64_t>;
using atomic_uint_fast64_t = atomic<std::uint_fast64_t>;

using atomic_int8_t = atomic<std::int8_t>;
using atomic_uint8_t = atomic<std::uint8_t>;
using atomic_int16_t = atomic<std::int16_t>;
using atomic_uint16_t = atomic<std::uint16_t>;
using atomic_int32_t = atomic<std::int32_t>;
using atomic_uint32_t = atomic<std::uint32_t>;
using atomic_int64_t = atomic<std::int64_t>;
using atomic_uint64_t = atomic<std::uint64_t>;

using atomic_intptr_t = atomic<std::intptr_t>;
using atomic_uintptr_t = atomic<std::uintptr_t>;
using atomic_size_t = atomic<std::size_t>;
using atomic_ptrdiff_t = atomic<std::ptrdiff_t>;
using atomic_intmax_t = atomic<std::intmax_t>;
using atomic_uintmax_t = atomic<std::uintmax_t>;

}  // namespace yaclib_std

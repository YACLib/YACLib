#pragma once

#include <yaclib/fault/detail/atomic/atomic.hpp>
#include <yaclib/fault/detail/atomic/atomic_flag.hpp>
#include <yaclib/fault/detail/atomic/atomic_reference.hpp>

namespace yaclib::std {

#ifdef YACLIB_FAULTY

template <class T>
using atomic = detail::Atomic<T>;

using atomic_flag = detail::AtomicFlag;

typedef detail::Atomic<bool> atomic_bool;
typedef detail::Atomic<char> atomic_char;
typedef detail::Atomic<signed char> atomic_schar;
typedef detail::Atomic<unsigned char> atomic_uchar;
typedef detail::Atomic<short> atomic_short;
typedef detail::Atomic<unsigned short> atomic_ushort;
typedef detail::Atomic<int> atomic_int;
typedef detail::Atomic<unsigned int> atomic_uint;
typedef detail::Atomic<long> atomic_long;
typedef detail::Atomic<unsigned long> atomic_ulong;
typedef detail::Atomic<long long> atomic_llong;
typedef detail::Atomic<unsigned long long> atomic_ullong;
typedef detail::Atomic<char16_t> atomic_char16_t;
typedef detail::Atomic<char32_t> atomic_char32_t;
typedef detail::Atomic<wchar_t> atomic_wchar_t;

typedef detail::Atomic<int_least8_t> atomic_int_least8_t;
typedef detail::Atomic<uint_least8_t> atomic_uint_least8_t;
typedef detail::Atomic<int_least16_t> atomic_int_least16_t;
typedef detail::Atomic<uint_least16_t> atomic_uint_least16_t;
typedef detail::Atomic<int_least32_t> atomic_int_least32_t;
typedef detail::Atomic<uint_least32_t> atomic_uint_least32_t;
typedef detail::Atomic<int_least64_t> atomic_int_least64_t;
typedef detail::Atomic<uint_least64_t> atomic_uint_least64_t;

typedef detail::Atomic<int_fast8_t> atomic_int_fast8_t;
typedef detail::Atomic<uint_fast8_t> atomic_uint_fast8_t;
typedef detail::Atomic<int_fast16_t> atomic_int_fast16_t;
typedef detail::Atomic<uint_fast16_t> atomic_uint_fast16_t;
typedef detail::Atomic<int_fast32_t> atomic_int_fast32_t;
typedef detail::Atomic<uint_fast32_t> atomic_uint_fast32_t;
typedef detail::Atomic<int_fast64_t> atomic_int_fast64_t;
typedef detail::Atomic<uint_fast64_t> atomic_uint_fast64_t;

typedef detail::Atomic<int8_t> atomic_int8_t;
typedef detail::Atomic<uint8_t> atomic_uint8_t;
typedef detail::Atomic<int16_t> atomic_int16_t;
typedef detail::Atomic<uint16_t> atomic_uint16_t;
typedef detail::Atomic<int32_t> atomic_int32_t;
typedef detail::Atomic<uint32_t> atomic_uint32_t;
typedef detail::Atomic<int64_t> atomic_int64_t;
typedef detail::Atomic<uint64_t> atomic_uint64_t;

typedef detail::Atomic<intptr_t> atomic_intptr_t;
typedef detail::Atomic<uintptr_t> atomic_uintptr_t;
typedef detail::Atomic<size_t> atomic_size_t;
typedef detail::Atomic<ptrdiff_t> atomic_ptrdiff_t;
typedef detail::Atomic<intmax_t> atomic_intmax_t;
typedef detail::Atomic<uintmax_t> atomic_uintmax_t;

#else

template <class T>
using atomic = ::std::atomic<T>;

using atomic_flag = ::std::atomic_flag;

typedef atomic<bool> atomic_bool;
typedef atomic<char> atomic_char;
typedef atomic<signed char> atomic_schar;
typedef atomic<unsigned char> atomic_uchar;
typedef atomic<short> atomic_short;
typedef atomic<unsigned short> atomic_ushort;
typedef atomic<int> atomic_int;
typedef atomic<unsigned int> atomic_uint;
typedef atomic<long> atomic_long;
typedef atomic<unsigned long> atomic_ulong;
typedef atomic<long long> atomic_llong;
typedef atomic<unsigned long long> atomic_ullong;
typedef atomic<char16_t> atomic_char16_t;
typedef atomic<char32_t> atomic_char32_t;
typedef atomic<wchar_t> atomic_wchar_t;

typedef atomic<int_least8_t> atomic_int_least8_t;
typedef atomic<uint_least8_t> atomic_uint_least8_t;
typedef atomic<int_least16_t> atomic_int_least16_t;
typedef atomic<uint_least16_t> atomic_uint_least16_t;
typedef atomic<int_least32_t> atomic_int_least32_t;
typedef atomic<uint_least32_t> atomic_uint_least32_t;
typedef atomic<int_least64_t> atomic_int_least64_t;
typedef atomic<uint_least64_t> atomic_uint_least64_t;

typedef atomic<int_fast8_t> atomic_int_fast8_t;
typedef atomic<uint_fast8_t> atomic_uint_fast8_t;
typedef atomic<int_fast16_t> atomic_int_fast16_t;
typedef atomic<uint_fast16_t> atomic_uint_fast16_t;
typedef atomic<int_fast32_t> atomic_int_fast32_t;
typedef atomic<uint_fast32_t> atomic_uint_fast32_t;
typedef atomic<int_fast64_t> atomic_int_fast64_t;
typedef atomic<uint_fast64_t> atomic_uint_fast64_t;

typedef atomic<int8_t> atomic_int8_t;
typedef atomic<uint8_t> atomic_uint8_t;
typedef atomic<int16_t> atomic_int16_t;
typedef atomic<uint16_t> atomic_uint16_t;
typedef atomic<int32_t> atomic_int32_t;
typedef atomic<uint32_t> atomic_uint32_t;
typedef atomic<int64_t> atomic_int64_t;
typedef atomic<uint64_t> atomic_uint64_t;

typedef atomic<intptr_t> atomic_intptr_t;
typedef atomic<uintptr_t> atomic_uintptr_t;
typedef atomic<size_t> atomic_size_t;
typedef atomic<ptrdiff_t> atomic_ptrdiff_t;
typedef atomic<intmax_t> atomic_intmax_t;
typedef atomic<uintmax_t> atomic_uintmax_t;

#endif
}  // namespace yaclib::std

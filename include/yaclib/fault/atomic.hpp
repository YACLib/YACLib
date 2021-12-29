#pragma once

#include <yaclib/fault/detail/atomic/atomic.hpp>
#include <yaclib/fault/detail/atomic/atomic_flag.hpp>
#include <yaclib/fault/detail/atomic/atomic_reference.hpp>

namespace yaclib_std {

#ifdef YACLIB_FAULT

template <typename T>
using atomic = yaclib::detail::Atomic<T>;

using atomic_flag = yaclib::detail::AtomicFlag;

// fences

inline void atomic_thread_fence(std::memory_order order) noexcept {
  YACLIB_INJECT_FAULT(std::atomic_thread_fence(order));
}

inline void atomic_signal_fence(std::memory_order order) noexcept {
  YACLIB_INJECT_FAULT(std::atomic_signal_fence(order));
}

#else

using std::atomic;

using atomic_flag = std::atomic_flag;

// fences

using atomic_fence_function_type = void (*)(std::memory_order);

inline constexpr atomic_fence_function_type atomic_thread_fence = std::atomic_thread_fence;

inline constexpr atomic_fence_function_type atomic_signal_fence = std::atomic_signal_fence;

#endif

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

using atomic_int_least8_t = atomic<int_least8_t>;
using atomic_uint_least8_t = atomic<uint_least8_t>;
using atomic_int_least16_t = atomic<int_least16_t>;
using atomic_uint_least16_t = atomic<uint_least16_t>;
using atomic_int_least32_t = atomic<int_least32_t>;
using atomic_uint_least32_t = atomic<uint_least32_t>;
using atomic_int_least64_t = atomic<int_least64_t>;
using atomic_uint_least64_t = atomic<uint_least64_t>;

using atomic_int_fast8_t = atomic<int_fast8_t>;
using atomic_uint_fast8_t = atomic<uint_fast8_t>;
using atomic_int_fast16_t = atomic<int_fast16_t>;
using atomic_uint_fast16_t = atomic<uint_fast16_t>;
using atomic_int_fast32_t = atomic<int_fast32_t>;
using atomic_uint_fast32_t = atomic<uint_fast32_t>;
using atomic_int_fast64_t = atomic<int_fast64_t>;
using atomic_uint_fast64_t = atomic<uint_fast64_t>;

using atomic_int8_t = atomic<int8_t>;
using atomic_uint8_t = atomic<uint8_t>;
using atomic_int16_t = atomic<int16_t>;
using atomic_uint16_t = atomic<uint16_t>;
using atomic_int32_t = atomic<int32_t>;
using atomic_uint32_t = atomic<uint32_t>;
using atomic_int64_t = atomic<int64_t>;
using atomic_uint64_t = atomic<uint64_t>;

using atomic_intptr_t = atomic<intptr_t>;
using atomic_uintptr_t = atomic<uintptr_t>;
using atomic_size_t = atomic<size_t>;
using atomic_ptrdiff_t = atomic<ptrdiff_t>;
using atomic_intmax_t = atomic<intmax_t>;
using atomic_uintmax_t = atomic<uintmax_t>;

}  // namespace yaclib_std

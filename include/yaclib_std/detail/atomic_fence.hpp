#pragma once

#if YACLIB_FAULT_ATOMIC_FENCE == 2

#  include <yaclib/fault/inject.hpp>

#  include <atomic>

namespace yaclib_std {

inline void atomic_thread_fence(std::memory_order /*order*/) noexcept {
}

inline void atomic_signal_fence(std::memory_order /*order*/) noexcept {
}

}  // namespace yaclib_std

#else
#  include <atomic>

namespace yaclib_std {

using std::atomic_signal_fence;
using std::atomic_thread_fence;

}  // namespace yaclib_std
#endif

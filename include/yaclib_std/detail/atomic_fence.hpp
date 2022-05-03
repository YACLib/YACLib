#pragma once

#if YACLIB_FAULT_ATOMIC_FENCE == 2

#  include <yaclib/fault/inject.hpp>

#  include <atomic>

namespace yaclib_std {

inline void atomic_thread_fence(std::memory_order order) noexcept {
  yaclib::InjectFault();
}

inline void atomic_signal_fence(std::memory_order order) noexcept {
  yaclib::InjectFault();
}

}  // namespace yaclib_std

#elif YACLIB_FAULT_ATOMIC_FENCE == 1
#  include <yaclib/fault/inject.hpp>

#  include <atomic>

namespace yaclib_std {

inline void atomic_thread_fence(std::memory_order order) noexcept {
  YACLIB_INJECT_FAULT(std::atomic_thread_fence(order));
}

inline void atomic_signal_fence(std::memory_order order) noexcept {
  YACLIB_INJECT_FAULT(std::atomic_signal_fence(order));
}

}  // namespace yaclib_std
#else
#  include <atomic>

namespace yaclib_std {

using std::atomic_signal_fence;
using std::atomic_thread_fence;

}  // namespace yaclib_std
#endif

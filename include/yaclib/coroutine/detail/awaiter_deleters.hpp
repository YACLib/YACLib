#pragma once
#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/ref.hpp>

namespace yaclib::detail {
struct Handle : public IRef {
  yaclib_std::coroutine_handle<> handle;
};

struct HandleDeleter {
  static void Delete(Handle& handle) noexcept;
};

struct LFStack : public IRef {
  yaclib_std::atomic<std::uintptr_t> head{kEmpty};
  bool AddWaiter(BaseCore* core_ptr) noexcept;
  constexpr static std::uintptr_t kEmpty = 0;
  constexpr static std::uintptr_t kAllDone = 1;
};
struct AwaitersResumer {
  static void Delete(LFStack& awaiters) noexcept;
};

}  // namespace yaclib::detail

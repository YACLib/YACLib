#pragma once
#include <yaclib/coroutine/coroutine.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/ref.hpp>

namespace yaclib::detail {
struct Handle : IRef {
  yaclib_std::coroutine_handle<> handle;
};

struct HandleDeleter {
  static void Delete(Handle& handle) noexcept;
};
}  // namespace yaclib::detail

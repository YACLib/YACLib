#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::detail {

class FutureAwaiter final {
 public:
  FutureAwaiter() = delete;
  FutureAwaiter(const FutureAwaiter&) = delete;
  FutureAwaiter(FutureAwaiter&&) = delete;
  FutureAwaiter& operator=(FutureAwaiter&&) = delete;
  FutureAwaiter& operator=(const FutureAwaiter&) = delete;
  ~FutureAwaiter() = default;

  template <typename... V, typename... E>
  explicit FutureAwaiter(Future<V, E>&... fs) : _await_core{1 + sizeof...(fs)} {
    auto wait_count = (... + static_cast<std::size_t>(fs.GetCore()->SetWait(_await_core)));
    _await_core.SubEqual(sizeof...(fs) - wait_count);
  }

  template <typename It, typename RangeIt>
  explicit FutureAwaiter(It it, RangeIt begin, RangeIt end) : _await_core{static_cast<std::size_t>(1 + end - begin)} {
    const std::size_t count = end - begin;
    std::size_t wait_count = 0;
    for (; begin != end; ++begin, ++it) {
      wait_count += it->GetCore()->SetWait(_await_core);
    }
    // TODO(mkornaukhov03, MBkkt) maybe relaxed?
    _await_core.SubEqual(count - wait_count);
  }

  bool await_ready() const noexcept {
    return _await_core.GetRef() == 1;
  }

  void await_resume() const noexcept {
  }

  bool await_suspend(yaclib_std::coroutine_handle<> handle) noexcept {
    _await_core.handle = std::move(handle);
    return !_await_core.SubEqual(1);
  }

 private:
  struct CoroHandler : IRef {
    yaclib_std::coroutine_handle<> handle;
  };
  struct CoroResumeDeleter {
    static void Delete(CoroHandler* handle) noexcept {
      assert(handle);
      assert(!handle->handle.done());
      handle->handle.resume();  // TODO(mkornaukhov03) resume on custom IExecutor
    }
  };
  detail::AtomicCounter<CoroHandler, CoroResumeDeleter> _await_core;
};

}  // namespace yaclib::detail

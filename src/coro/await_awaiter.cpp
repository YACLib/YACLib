#include <yaclib/coro/detail/await_awaiter.hpp>

namespace yaclib::detail {

void AwaitAwaiter::HandleDeleter::Delete(AwaitAwaiter::Handle& handle) noexcept {
  YACLIB_DEBUG(!handle.handle, "saved to resume handle is null");
  YACLIB_DEBUG(handle.handle.done(), "handle for resume is done");
  handle.handle.resume();  // TODO(mkornaukhov03) resume on custom IExecutor
}

}  // namespace yaclib::detail

#include <yaclib/coroutine/detail/handle_wrapper.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

void HandleDeleter::Delete(Handle& handle) noexcept {
  YACLIB_DEBUG(!handle.handle, "saved to resume handle is null");
  YACLIB_DEBUG(handle.handle.done(), "handle for resume is done");
  handle.handle.resume();  // TODO(mkornaukhov03) resume on custom IExecutor
}

}  // namespace yaclib::detail

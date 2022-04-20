#include <yaclib/coroutine/via.hpp>

namespace yaclib {

detail::ViaAwaiter Via(IExecutor& e) {
  return detail::ViaAwaiter(e);
}

}  // namespace yaclib

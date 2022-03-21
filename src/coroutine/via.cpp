#include <yaclib/coroutine/via.hpp>

namespace yaclib {
detail::ViaAwaiter Via(IExecutorPtr e) {
  return detail::ViaAwaiter(std::move(e));
}
}  // namespace yaclib

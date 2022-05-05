#include <yaclib/coro/on.hpp>

namespace yaclib {

detail::OnAwaiter On(IExecutor& e) {
  return detail::OnAwaiter(e);
}

}  // namespace yaclib

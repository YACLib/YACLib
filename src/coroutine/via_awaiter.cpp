#include <yaclib/coroutine/detail/via_awaiter.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>

namespace yaclib::detail {

ViaAwaiter::ViaAwaiter(IExecutor& e) : _executor{e} {
}

}  // namespace yaclib::detail

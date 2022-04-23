#include <yaclib/coroutine/detail/on_awaiter.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/job.hpp>

namespace yaclib::detail {

OnAwaiter::OnAwaiter(IExecutor& e) : _executor{e} {
}

}  // namespace yaclib::detail

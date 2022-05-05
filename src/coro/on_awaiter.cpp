#include <yaclib/coro/detail/on_awaiter.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>

namespace yaclib::detail {

OnAwaiter::OnAwaiter(IExecutor& e) : _executor{e} {
}

}  // namespace yaclib::detail

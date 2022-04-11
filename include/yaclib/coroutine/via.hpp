#pragma once

#include <yaclib/coroutine/detail/via_awaiter.hpp>
#include <yaclib/executor/executor.hpp>

namespace yaclib {

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
detail::ViaAwaiter Via(IExecutor& e);

}  // namespace yaclib

#pragma once

#include <yaclib/coroutine/detail/via_awaiter.hpp>
#include <yaclib/executor/executor.hpp>

namespace yaclib {
detail::ViaAwaiter Via(IExecutorPtr e);
}  // namespace yaclib

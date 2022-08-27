#pragma once

#include <yaclib/config.hpp>

#if YACLIB_FUTEX != 0
#  include <yaclib/util/detail/atomic_event.hpp>
#else
#  include <yaclib/util/detail/mutex_event.hpp>
#endif

namespace yaclib::detail {

#if YACLIB_FUTEX != 0
using DefaultEvent = AtomicEvent;
#else
using DefaultEvent = MutexEvent;
#endif

}  // namespace yaclib::detail

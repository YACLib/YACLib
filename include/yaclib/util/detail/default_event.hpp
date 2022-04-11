#pragma once

#include <yaclib/config.hpp>

#ifdef YACLIB_ATOMIC_EVENT
#  include <yaclib/util/detail/atomic_event.hpp>
#else
#  include <yaclib/util/detail/mutex_event.hpp>
#endif

namespace yaclib::detail {

#ifdef YACLIB_ATOMIC_EVENT
using DefaultEvent = AtomicEvent;
#else
using DefaultEvent = MutexEvent;
#endif

}  // namespace yaclib::detail

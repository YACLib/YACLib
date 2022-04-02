#include <yaclib/algo/detail/wait_impl.hpp>
#include <yaclib/config.hpp>

namespace yaclib::detail {

template bool WaitCore<detail::MutexEvent, NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);
#ifdef YACLIB_ATOMIC_EVENT
template bool WaitCore<detail::AtomicEvent, NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);
#endif

}  // namespace yaclib::detail

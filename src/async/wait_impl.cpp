#include <yaclib/async/detail/wait_impl.hpp>

namespace yaclib::detail {

template bool WaitCore<DefaultEvent, NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&) noexcept;
template bool WaitCore<DefaultEvent, NoTimeoutTag, BaseCore, BaseCore>(const NoTimeoutTag&, BaseCore&,
                                                                       BaseCore&) noexcept;

}  // namespace yaclib::detail

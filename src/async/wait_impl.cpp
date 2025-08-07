#include <yaclib/async/detail/wait_impl.hpp>

namespace yaclib::detail {

template bool WaitCore<DefaultEvent, NoTimeoutTag, UniqueHandle>(const NoTimeoutTag&, UniqueHandle) noexcept;
template bool WaitCore<DefaultEvent, NoTimeoutTag, SharedHandle>(const NoTimeoutTag&, SharedHandle) noexcept;

}  // namespace yaclib::detail

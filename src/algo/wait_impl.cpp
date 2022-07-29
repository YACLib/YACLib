#include <yaclib/algo/detail/wait_impl.hpp>

namespace yaclib::detail {

template bool WaitCore<DefaultEvent, NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&) noexcept;

}  // namespace yaclib::detail

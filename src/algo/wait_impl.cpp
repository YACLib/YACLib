#include <yaclib/algo/detail/wait_impl.hpp>

namespace yaclib::detail {

template bool WaitCores<NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);

}  // namespace yaclib::detail

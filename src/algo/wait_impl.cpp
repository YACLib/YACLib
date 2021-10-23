#include <yaclib/algo/detail/wait_impl.hpp>

namespace yaclib::detail {

template bool Wait<NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);

}  // namespace yaclib::detail

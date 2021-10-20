#include <yaclib/algo/detail/wait.hpp>

namespace yaclib::detail {

template bool Wait<NoTimeoutTag, BaseCore>(const NoTimeoutTag&, BaseCore&);

}  // namespace yaclib::detail

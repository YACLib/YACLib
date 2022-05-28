#include <yaclib/algo/detail/wait_impl.hpp>

namespace yaclib::detail {

template bool WaitCore<DefaultEvent, NoTimeoutTag, CCore>(const NoTimeoutTag&, CCore&);

}  // namespace yaclib::detail

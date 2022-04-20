#include <yaclib/async/future.hpp>

namespace yaclib {

template class FutureBase<void, StopError>;
template class Future<void, StopError>;
template class FutureOn<void, StopError>;

}  // namespace yaclib

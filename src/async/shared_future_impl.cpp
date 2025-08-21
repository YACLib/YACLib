#include <yaclib/async/shared_future.hpp>

namespace yaclib {

template class SharedFutureBase<void, StopError>;
template class SharedFuture<>;
template class SharedFutureOn<>;

}  // namespace yaclib

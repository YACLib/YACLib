#include <yaclib/async/future.hpp>

namespace yaclib {

template class FutureBase<void, StopError>;
template class Future<>;
template class FutureOn<>;

}  // namespace yaclib

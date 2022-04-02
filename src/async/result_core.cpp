#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/config.hpp>

namespace yaclib::detail {

template class ResultCore<void, StopError>;

}  // namespace yaclib::detail

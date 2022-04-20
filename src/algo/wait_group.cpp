#include <yaclib/algo/wait_group.hpp>

namespace yaclib {

template class WaitGroup<1, detail::DefaultEvent>;

}  // namespace yaclib

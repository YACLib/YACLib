#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {

void Run(BaseCore* head, IExecutor& e) noexcept {
  head = MoveToCaller(head);
  head->_executor = &e;
  e.Submit(*head);
}

void Run(BaseCore* head) noexcept {
  head = MoveToCaller(head);
  head->_executor->Submit(*head);
}

}  // namespace detail

template class Task<>;

}  // namespace yaclib

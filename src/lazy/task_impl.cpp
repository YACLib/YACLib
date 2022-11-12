#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {

void Start(BaseCore* head, IExecutor& e) noexcept {
  head = MoveToCaller(head);
  head->_executor = &e;
  e.Submit(*head);
}

void Start(BaseCore* head) noexcept {
  head = MoveToCaller(head);
  head->_executor->Submit(*head);
}

}  // namespace detail

template class Task<>;

}  // namespace yaclib

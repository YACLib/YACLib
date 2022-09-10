#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {
namespace {

BaseCore* MoveToCaller(BaseCore* head) noexcept {
  while (head->next != nullptr) {
    auto* next = static_cast<BaseCore*>(head->next);
    head->next = nullptr;
    head = next;
  }
  return head;
}

}  // namespace

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

template class Task<void, StopError>;

}  // namespace yaclib

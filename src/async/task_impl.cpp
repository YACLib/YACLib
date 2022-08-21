#include <yaclib/async/task.hpp>

namespace yaclib {
namespace detail {
namespace {

template <bool Force>
bool MoveToCaller(BaseCore*& iterator, IExecutor& e) noexcept {
  if (iterator->next == nullptr) {
    return false;
  }
  auto* caller = static_cast<BaseCore*>(iterator->next);
  iterator->next = nullptr;
  if (Force || !caller->_executor) {
    caller->_executor = &e;
  }
  iterator = caller;
  return true;
}

}  // namespace

template <bool Force>
void Run(BaseCore* head, IExecutor& e) noexcept {
  while (MoveToCaller<Force>(head, e)) {
  }
  auto* head_executor = static_cast<IExecutor*>(head->_caller);
  if (head_executor == nullptr) {
    e.IncRef();
    head->_caller = head_executor = &e;
  }
  if constexpr (Force) {
    e.Submit(*head);
  } else {
    head_executor->Submit(*head);
  }
}

template void Run<false>(detail::BaseCore* head, IExecutor& e) noexcept;
template void Run<true>(detail::BaseCore* head, IExecutor& e) noexcept;

}  // namespace detail

template class Task<void, StopError>;

}  // namespace yaclib

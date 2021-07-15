#include "intrusive_list.hpp"

#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/task.hpp>

namespace yaclib::container::intrusive {

template <typename T>
T* List<T>::PopBack() noexcept {
  if (IsEmpty()) {
    return nullptr;
  }
  auto* back = _head._prev;
  back->Unlink();
  return back->AsItem();
}

template <typename T>
T* List<T>::PopFront() noexcept {
  if (IsEmpty()) {
    return nullptr;
  }
  auto* front = _head._next;
  front->Unlink();
  return front->AsItem();
}

template class List<ITask>;
template class List<executor::IThread>;

}  // namespace yaclib::container::intrusive

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
  return AsItem(back);
}

template <typename T>
T* List<T>::PopFront() noexcept {
  if (IsEmpty()) {
    return nullptr;
  }
  auto* front = _head._next;
  front->Unlink();
  return AsItem(front);
}

template <typename T>
void List<T>::Append(List& other) noexcept {
  if (other.IsEmpty()) {
    return;
  }
  auto* other_front = other._head._next;
  auto* other_back = other._head._prev;

  // insert to end
  other_back->_next = &_head;
  other_front->_prev = _head._prev;

  _head._prev->_next = other_front;
  _head._prev = other_back;

  // clear other
  other._head._next = &other._head;
  other._head._prev = &other._head;
}

template class List<ITask>;
template class List<executor::IThread>;

}  // namespace yaclib::container::intrusive

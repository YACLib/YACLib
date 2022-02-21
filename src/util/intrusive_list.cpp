#include <util/intrusive_list.hpp>

#include <utility>

namespace yaclib {

template <typename T>
List<T>::List(List&& other) noexcept {
  if (this == &other || other.Empty()) {
    return;
  }
  _head.next = std::exchange(other._head.next, nullptr);
  _tail = std::exchange(other._tail, &other._head);
}

template <typename T>
void List<T>::PushFront(detail::Node& node) noexcept {
  if (Empty()) {
    _tail = &node;
  }
  node.next = _head.next;
  _head.next = &node;
}
template <typename T>
void List<T>::PushBack(detail::Node& node) noexcept {
  // for circular should be node.next = _tail->next;
  assert(_tail->next == nullptr);
  node.next = nullptr;
  _tail->next = &node;
  _tail = &node;
}

template <typename T>
bool List<T>::Empty() const noexcept {
  assert((_head.next == nullptr) == (_tail == &_head));
  return _head.next == nullptr;  // valid only for linear
}
template <typename T>
T& List<T>::PopFront() noexcept {
  assert(!Empty());
  auto* node = _head.next;
  _head.next = node->next;
  if (node->next == nullptr) {  // valid only for linear
    _tail = &_head;
  }
  return static_cast<T&>(*node);
}

template class List<ITask>;
template class List<IThread>;

}  // namespace yaclib

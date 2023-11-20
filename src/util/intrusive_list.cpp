#include <yaclib/log.hpp>
#include <yaclib/util/detail/intrusive_list.hpp>

#include <utility>

namespace yaclib::detail {

List::List(List&& other) noexcept {
  if (this == &other || other.Empty()) {
    return;  // TODO(MBkkt) Remove if
  }
  _head.next = std::exchange(other._head.next, nullptr);
  _tail = std::exchange(other._tail, &other._head);
}

void List::PushFront(Node& node) noexcept {
  if (Empty()) {
    _tail = &node;
  }
  node.next = _head.next;
  _head.next = &node;
}

void List::PushBack(Node& node) noexcept {
  // for circular should be node.next = _tail->next;
  node.next = nullptr;
  _tail->next = &node;
  _tail = &node;
}

bool List::Empty() const noexcept {
  YACLIB_DEBUG((_head.next == nullptr) != (_tail == &_head), "List::Empty invariant is failed");
  return _head.next == nullptr;  // valid only for linear
}

Node& List::PopFront() noexcept {
  YACLIB_ASSERT(!Empty());
  auto* node = _head.next;
  _head.next = node->next;
  if (_head.next == nullptr) {
    _tail = &_head;
  }
  return *node;
}

}  // namespace yaclib::detail
